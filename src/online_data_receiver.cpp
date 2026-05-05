#include "include/online_data_receiver.h"

#include <string>
#include <iostream>
#include <fstream>

void ReadOnlineInformation(
	int &run,
	int &crate,
	size_t &module_num,
	std::vector<int> &module_sampling_rate,
	std::vector<int> &group_index
) {
	module_sampling_rate.clear();
	group_index.clear();
	// get file name
	std::string file_name =
		std::string(getenv("HOME"))
		+ "/.xia-daq-gui-online/online_information.txt";
	// input file stream
	std::ifstream fin(file_name);
	fin >> run >> crate >> module_num;
	int tmp;
	for (size_t i = 0; i < module_num; ++i) {
		fin >> tmp;
		module_sampling_rate.push_back(tmp);
	}
	for (size_t i = 0; i < module_num; ++i) {
		fin >> tmp;
		group_index.push_back(tmp);
	}
	// close file
	fin.close();
}


OnlineDataReceiver::OnlineDataReceiver(
	const char *app_name,
	const char *service_name
) {
	char name[32];
	strcpy(name, app_name);
	// create runtime
	iox::runtime::PoshRuntime::initRuntime(name);

	// get online information
	int run, crate;
	ReadOnlineInformation(
		run, crate, module_num_,
		sampling_rate_, group_index_
	);

	// subscriber options
	iox_sub_options_t options;
    iox_sub_options_init(&options);
    options.queueCapacity = 40U;
    options.historyRequest = 5U;
	options.queueFullPolicy = QueueFullPolicy_DISCARD_OLDEST_DATA;
    options.nodeName = "online-node";

	// initialize subscribers
	std::string run_name = "run" + std::to_string(run);
	for (size_t i = 0; i < module_num_; ++i) {
		std::string module_name =
			"c" + std::to_string(crate) + "m" + std::to_string(i);
		subscriber_[i] = iox_sub_init(
			subscriber_storage_+i,
			service_name, run_name.c_str(), module_name.c_str(),
			&options
		);
	}

	// initialize payload
	for (size_t i = 0; i < module_num_; ++i) {
		user_payload_[i] = nullptr;
		header_[i] = nullptr;
		packet_[i] = nullptr;
	}

	has_taken_.clear();

	for (size_t i = 0; i < module_num_; ++i) {
		first_events_[i].used = true;
	}


	// intialize group information
	for (size_t i = 0; i < 16; ++i) {
		group_info_[i].size = 0;
		group_info_[i].valid_packets = 0;
		group_info_[i].expect_id = 0;
	}
	for (size_t i = 0; i < module_num_; ++i) {
		if (group_index_[i] < 0) continue;
		++group_info_[group_index_[i]].size;
		valid_group_index_.insert(group_index_[i]);
	}
}


std::vector<DecodeEvent>* OnlineDataReceiver::ReceiveEvent(
	const int64_t window
) {
	if (has_taken_.empty()) {
		for (size_t module = 0; module < module_num_; ++module) {
			if (user_payload_[module]) continue;
			// take
			enum iox_ChunkReceiveResult take_result = iox_sub_take_chunk(
				subscriber_[module], user_payload_+module
			);
			// no available chunk, continue
			if (take_result == ChunkReceiveResult_NO_CHUNK_AVAILABLE) continue;

			// take chunk error
			if (take_result != ChunkReceiveResult_SUCCESS) {
				std::cerr << "Warning: Take payload failed: "
					<< take_result << "\n";
				continue;
			}
			// get header
			header_[module] = (const PacketHeader*)(
				iox_chunk_header_to_user_header_const(
					iox_chunk_header_from_user_payload_const(
						user_payload_[module]
					)
				)
			);
			// check packet id
			uint64_t &expected_id =
				group_info_[group_index_[module]].expect_id;
			if (header_[module]->id > expected_id) {
				// packet id larger than expected, expected id is out dated
				// update expected id and reset valid packet number to 1
				expected_id = header_[module]->id;
				group_info_[group_index_[module]].valid_packets = 1;
				// release out dated packet
				for (size_t i = 0; i < module_num_; ++i) {
					// ignore other group
					if (group_index_[i] != group_index_[module]) continue;
					// ignore empty chunk
					if (!user_payload_[i]) continue;
					// ignore valid packet
					if (header_[i]->id >= expected_id) continue;
					// release out dated packet
					iox_sub_release_chunk(subscriber_[i], user_payload_[i]);
					user_payload_[i] = nullptr;
					header_[i] = nullptr;
					packet_[i] = nullptr;
				}
			} else if (header_[module]->id < expected_id) {
				// packet id smaller than expected, packet is out dated, release it
				iox_sub_release_chunk(subscriber_[module], user_payload_[module]);
				user_payload_[module] = nullptr;
				header_[module] = nullptr;
				packet_[module] = nullptr;
				continue;
			} else {
				// packet id is equal to expected id
				++group_info_[group_index_[module]].valid_packets;
			}
			packet_[module] = (const DaqPacket*)user_payload_[module];
			first_events_[module].used = true;
			decode_offset_[module] = 0;
		}

		// check all groups, record taken group
		for (size_t g : valid_group_index_) {
			if (group_info_[g].valid_packets == group_info_[g].size) {
				has_taken_.push_back(int(g));
			}
		}
		if (has_taken_.empty()) return nullptr;
	}

// if (!has_taken_) {
// 	for (int m = 0; m < 4; ++m) {
// 		for (int j = 0; j < 4; ++j) {
// 			std::cout
// 				<< "Module " << m
// 				<< "  timestamp low " << packet_[m]->data[4*j+1]
// 				<< "  timestamp high " << (packet_[m]->data[4*j+2] & 0xffff)
// 				<< ", cfd " << ((packet_[m]->data[4*j+2] >> 16) & 0xffff) << "\n";
// 		}
// 	}
// }

	// timestamps of first events
	int64_t ref_timestamp;
	// initialize
	event_.clear();

	bool finish = false;
	while (!finish) {
		finish = true;
		// get first events
		for (size_t i = 0; i < module_num_; ++i) {
			if (group_index_[i] != has_taken_.back()) continue;
			if (
				first_events_[i].used
				&& decode_offset_[i]+sizeof(DataHeader)/4 < header_[i]->length
			) {
				Decode(
					packet_[i]->data,
					decode_offset_[i],
					sampling_rate_[i],
					first_events_[i]
				);
			}
		}

		// check data
		bool has_data = false;
		for (size_t i = 0; i < module_num_; ++i) {
			if (group_index_[i] != has_taken_.back()) continue;
			if (
				decode_offset_[i]+sizeof(DataHeader)/4 < header_[i]->length
				|| !first_events_[i].used
			) {
				has_data = true;
			}
		}
		if (!has_data) {
			// clean
			for (size_t i = 0; i < module_num_; ++i) {
				if (group_index_[i] != has_taken_.back()) continue;
				iox_sub_release_chunk(subscriber_[i], user_payload_[i]);
				user_payload_[i] = nullptr;
				header_[i] = nullptr;
				packet_[i] = nullptr;
			}
			group_info_[has_taken_.back()].valid_packets = 0;
			++group_info_[has_taken_.back()].expect_id;
			has_taken_.pop_back();
			return nullptr;
		}

		if (event_.empty()) {
			// new event, find the minimum timestamp
			// minimum timestamp
			int64_t min_ts = 0x7fff'ffff'ffff'ffff;
			// module with minimum timestamp
			size_t min_ts_index = 16;
			for (size_t i = 0; i < module_num_; ++i) {
				if (group_index_[i] != has_taken_.back()) continue;
				if (first_events_[i].used) continue;
				if (first_events_[i].timestamp < min_ts) {
					min_ts = first_events_[i].timestamp;
					min_ts_index = i;
				}
			}
			if (min_ts_index == 16) {
				std::cerr << "Error: Could not find minimum timestamp.\n";
				break;
			}
			finish = false;
			first_events_[min_ts_index].used = true;
			ref_timestamp = first_events_[min_ts_index].timestamp;
			// fill to fundamental event
			event_.push_back(first_events_[min_ts_index]);

		} else {
			for (size_t i = 0; i < module_num_; ++i) {
				if (group_index_[i] != has_taken_.back()) continue;
				if (first_events_[i].used) continue;
				if (
					first_events_[i].timestamp - ref_timestamp > -window
					&& first_events_[i].timestamp - ref_timestamp < window
				) {
					finish = false;
					first_events_[i].used = true;
					event_.push_back(first_events_[i]);
				}
			}
		}
	}

	if (event_.empty()) return nullptr;
	return &event_;
}


bool OnlineDataReceiver::Alive() const {
	return !iox::posix::hasTerminationRequested();
}



void OnlineDataReceiver::Decode(
	const unsigned int *data,
	size_t &offset,
	const int rate,
	DecodeEvent &event
) {
	const DataHeader *header = (const DataHeader*)(data + offset);
	// get event length and increase offset
	int event_length = (header->data[0] >> 17) & 0x3fff;
	offset += event_length;
	// fill information
	event.module = (unsigned short)(((header->data[0] >> 4) & 0xf) - 2);
	event.channel = header->data[0] & 0xf;
	event.energy = header->data[3] & 0xffff;
	// cfd
	event.cfd = 0.0;
	if (rate == 100) {
		bool cfd_force = (header->data[2] >> 31) != 0;
		if (!cfd_force) {
			event.cfd = double((header->data[2] >> 16) & 0x7fff);
			event.cfd = event.cfd / 32768.0 * 10.0;
		}
	} else if (rate == 250) {
		bool cfd_force = (header->data[2] >> 31) != 0;
		if (!cfd_force) {
			unsigned int cfds = (header->data[2] >> 30) & 0x1;
			event.cfd = double((header->data[2] >> 16) & 0x3fff);
			event.cfd = (event.cfd / 16384.0 - cfds) * 4.0;
		}
	} else { 					//500
		unsigned int cfds = (header->data[2] >> 29) & 0x7;
		bool cfd_force = cfds == 7;
		if (!cfd_force) {
			event.cfd = double((header->data[2] >> 16) & 0x1fff);
			event.cfd = (event.cfd / 8192.0 + cfds - 1) * 2.0;
		}
	}
	// timestamp
	event.timestamp = int64_t(header->data[2] & 0xffff) << 32;
	event.timestamp |= header->data[1];
	if (rate == 250) event.timestamp *= 8;
	else event.timestamp *= 10;
	// time
	event.time = double(event.timestamp) + event.cfd;
	// used
	event.used = false;
	return;
}
