#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>

#include <TRandom3.h>

#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>
#if defined (__cplusplus)
extern "C" {
#endif
#include <iceoryx_binding_c/chunk.h>
#include <iceoryx_binding_c/runtime.h>
#include <iceoryx_binding_c/publisher.h>
#if defined (__cplusplus)
}
#endif

#include "include/daq_packet.h"

#include "alpha_source_dssd_global.h"

// alpha source settings
// energy of alpha source, in MeV
// 典型 3α 源的能量
constexpr double alpha_energy[3] = {5.157, 5.486, 5.805};
// counting rate, event per second
constexpr int event_rate = 1000;

// DAQ setting
// dead time of DAQ, in seconds
constexpr double dead_time = 1e-6;
// module number
constexpr size_t module_number = 4;

// DSSD settings
// DSSD distance from source, in mm
constexpr double dssd_distance = 10.0;
// DSSD size, in mm
constexpr double dssd_size = 64.0;
// DSSD strips
constexpr size_t dssd_strips = 32;
// DSSD energy resolution
constexpr double dssd_resolution = 0.01;
// DSSD time resultion, in seconds
constexpr double dssd_time_resolution = 1.2e-9;

// intrisic frame per seconds, 逻辑帧
constexpr int fps = 100;


/// @brief save online needed information to file
/// @param[in] run run number
/// @param[in] crate crate number
/// @param[in] sampling_rate module sampling rate
/// @param[in] group_index group index of module
/// @param[in] module_slot module slot, start from 2
void SaveOnlineInformation(
	const int run,
	const int crate,
	const std::vector<unsigned int> &sampling_rate,
	const std::vector<unsigned int> &group_index,
	const std::vector<unsigned int> &module_slot
) {
	// get directory path
	std::string path = std::string(getenv("HOME")) + "/.xia-daq-gui-online";
	// create directory
	std::filesystem::create_directories(path);
	// open file
	std::ofstream fout(path+"/online_information.txt");
	fout << run << "\n"
		<< crate << "\n"
		<< sampling_rate.size() << "\n";
    for (size_t i = 0; i < sampling_rate.size(); ++i) {
		fout << sampling_rate[i] << " \n"[i==sampling_rate.size()-1];
	}
	for (size_t i = 0; i < group_index.size(); ++i) {
		fout << group_index[i] << " \n"[i==group_index.size()-1];
	}
	for (size_t i = 0; i < module_slot.size(); ++i) {
		fout << module_slot[i] << " \n"[i==module_slot.size()-1];
	}
	// close file
	fout.close();
}


void FillBinaryEvent(
	DataHeader &header,
	int channel,
	int slot,
	uint64_t timestamp,
	uint32_t cfd,
	uint32_t energy
) {
	// fill x event
	memset(&header, 0, sizeof(DataHeader));
	// channel
	header.data[0] |= channel & 0xf;
	// slot
	header.data[0] |= (slot & 0xf) << 4;
	// crate ID
	header.data[0] |= 0 << 8;
	// header length
	header.data[0] |= 4 << 12;
	// event length
	header.data[0] |= 4 << 17;
	// timemstamp low 32 bits
	header.data[1] = uint32_t(timestamp & 0xffffffff);
	// timestamp high 16 bits
	header.data[2] |= uint32_t((timestamp >> 32) & 0xffff);
	// CFD
	header.data[2] |= (cfd & 0x7fff) << 16;
	// energy
	if (energy >= 65536) energy = 65535;
	header.data[3] |= energy & 0xffff;
}


void AllocatePayload(
	const iox_pub_t &publisher,
	void **user_payload,
	PacketHeader **header,
	DaqPacket **packet
) {
	const uint32_t ALIGNMENT = 8;
	// allocate shared memeory
	enum iox_AllocationResult res = iox_pub_loan_aligned_chunk_with_user_header(
		publisher, user_payload,
		sizeof(DaqPacket), ALIGNMENT,
		sizeof(PacketHeader), ALIGNMENT
	);
	if (res != AllocationResult_SUCCESS) {
		// allocate memory failed
		*user_payload = nullptr;
		*header = nullptr;
		*packet = nullptr;
		std::cout << "[Warn] Failed to allocate user payload! "
			<< "Error code " << int(res) << "\n";
	}
	// success, get header pointer
	*header = (PacketHeader*)iox_chunk_header_to_user_header(
		iox_chunk_header_from_user_payload(*user_payload)
	);
	// initialize length
	(*header)->length = 0;
	// get packet pointer
	*packet = (DaqPacket*)(*user_payload);
}


int main() {
	// assert
	if (event_rate < fps) {
		std::cerr << "[Error] Event rate must be greater than FPS"
			<< ", event rate " << event_rate
			<< ", fps " << fps << "\n";
		return -1;
	}

	constexpr char app_name[] = "simulate_online_example";

	// run number
	int run = 0;
	// crate ID
	int crate = 0;

	// init run time
	iox_runtime_init(app_name);

	// create publisher with some options set
    iox_pub_options_t publisher_options;
	iox_pub_options_init(&publisher_options);
	publisher_options.historyCapacity = 10U;
	publisher_options.nodeName = "DAQ_online_packet";
	publisher_options.subscriberTooSlowPolicy =
		iox_ConsumerTooSlowPolicy::ConsumerTooSlowPolicy_DISCARD_OLDEST_DATA;

	iox_pub_storage_t storage[module_number];
	iox_pub_t publishers[module_number];
	void *user_payload[module_number];
	PacketHeader *packet_header[module_number];
	DaqPacket *packet[module_number];
	uint64_t packet_id = 0;
	for (size_t i = 0; i < module_number; ++i) {
		user_payload[i] = nullptr;
		packet_header[i] = nullptr;
		packet[i] = nullptr;
	}

	// randon number generator
	TRandom3 generator(0);

	for (size_t i = 0; i < module_number; ++i) {
		std::string run_name = "run" + std::to_string(run);
		std::string module_name =
			"c" + std::to_string(crate) + "m" + std::to_string(i);
		publishers[i] = iox_pub_init(
			storage+i,
			"ExampleSimulateOnline",
			run_name.c_str(),
			module_name.c_str(),
			&publisher_options
		);
	}

	std::vector<unsigned int> sampling_rate = {100, 100, 100, 100};
	std::vector<unsigned int> group_index = {0, 0, 0, 0};
	std::vector<unsigned int> module_slot = {2, 3, 4, 5};
	SaveOnlineInformation(run, crate, sampling_rate, group_index, module_slot);

	// accumulated interval time
	double accumulated_interval = 0.0;
	// time of event
	double detect_time = 0.0;

	// preparation of simulatios
	// DSSD max theta
	const double dssd_max_theta =
		atan(dssd_size / sqrt(2.0) / dssd_distance);

	// start time
	auto last_time = std::chrono::steady_clock::now();

	// number generate events
	uint64_t num_events = 0;

	while (!iox::posix::hasTerminationRequested()) {
		// increase number of events
		++num_events;

		// generate event
		// energy
		int peak = int(generator.Rndm() * 3);
		double energy = alpha_energy[peak];
		// time
		// interval time between two events, in seconds
		double interval = -log(generator.Rndm()) / event_rate;
		accumulated_interval += interval;
		if (interval >= dead_time) {
			detect_time += accumulated_interval;
			accumulated_interval = 0.0;
		}
		// position
		double theta = acos(generator.Rndm()*2.0-1.0);
		double phi = 2.0 * M_PI * generator.Rndm();


		// simulate DSSD detect
		// check position
		if (theta > dssd_max_theta) continue;
		double x = dssd_distance * tan(theta) * cos(phi);
		double y = dssd_distance * tan(theta) * sin(phi);
		if (fabs(x) > 32.0 || fabs(y) > 32.0) continue;
		// get strip
		int x_strip = int((x + 32.0) / 2.0);
		int y_strip = int((y + 32.0) / 2.0);

		// consider energy resolution
		double x_energy = energy + generator.Gaus(
			0.0,
			energy * dssd_resolution / (2.0*sqrt(2.0*log(2.0)))
		);
		double y_energy = energy + generator.Gaus(
			0.0,
			energy * dssd_resolution / (2.0*sqrt(2.0*log(2.0)))
		);

		// consider time resolution
		double x_time = detect_time + generator.Gaus(
			0.0,
			dssd_time_resolution / (2.0*sqrt(2.0*log(2.0)))
		);
		double y_time = detect_time + generator.Gaus(
			0.0,
			dssd_time_resolution / (2.0*sqrt(2.0*log(2.0)))
		);


		// convert strip to module and channel
		int module[2];
		module[0] = x_strip < 16 ? 0 : 1;
		module[1] = y_strip < 16 ? 2 : 3;
		int channel[2];
		channel[0] = x_strip % 16;
		channel[1] = y_strip % 16;

		// convert energy to channel
		double xp0 = calibration_parameters[x_strip][1];
		double xp1 = calibration_parameters[x_strip][0];
		double yp0 = calibration_parameters[y_strip+dssd_strips][1];
		double yp1  = calibration_parameters[y_strip+dssd_strips][0];
		double energy_channel[2];
		energy_channel[0] = (x_energy - xp0) / xp1;
		energy_channel[1] = (y_energy - yp0) / yp1;

		// convert time to timestamp and cfd
		uint64_t timestamp[2];
		uint32_t cfd[2];
		timestamp[0] = uint64_t(x_time * 1e8);
		timestamp[1] = uint64_t(y_time * 1e8);
		cfd[0] = uint32_t((x_time*1e8 - double(timestamp[0])) * 32768.0);
		cfd[1] = uint32_t((y_time*1e8 - double(timestamp[1])) * 32768.0);


		// construct binary event and store
		DataHeader header;
		for (size_t i = 0; i < 2; ++i) {
			FillBinaryEvent(
				header,
				channel[i], module[i]+2,
				timestamp[i], cfd[i],
				uint32_t(energy_channel[i])
			);
			if (!user_payload[module[i]]) {
				AllocatePayload(
					publishers[module[i]],
					user_payload + module[i],
					packet_header + module[i],
					packet + module[i]
				);
			}
			if (
				packet_header[module[i]]->length + sizeof(DataHeader)/4 > PACKET_SIZE
			) {
				for (size_t m = 0; m < module_number; ++m) {
// for (int j = 0; j < 4; ++j) {
// 	std::cout
// 		<< "Module " << m
// 		<< "  timestamp low " << packet[m]->data[4*j+1]
// 		<< "  timestamp high " << (packet[m]->data[4*j+2] & 0xffff)
// 		<< ", cfd " << ((packet[m]->data[4*j+2] >> 16) & 0xffff) << "\n";
// }
					// fill packet ID
					packet_header[m]->id = packet_id;
					// publish
					iox_pub_publish_chunk(
						publishers[m], user_payload[m]
					);
					// get new payload
					AllocatePayload(
						publishers[m],
						user_payload + m,
						packet_header + m,
						packet + m
					);
				}
				++packet_id;
			}
			memcpy(
				(void*)(packet[module[i]]->data + packet_header[module[i]]->length),
				(void*)&header,
				sizeof(DataHeader)
			);
			packet_header[module[i]]->length += sizeof(DataHeader) / 4;
		}

		if (num_events % (event_rate / fps) == 0) {
			std::this_thread::sleep_until(
				last_time+std::chrono::milliseconds(1000/fps)
			);
			last_time = std::chrono::steady_clock::now();
		}
	}

	return 0;
}