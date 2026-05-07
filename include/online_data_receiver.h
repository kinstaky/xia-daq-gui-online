#ifndef __ONLINE_DATA_RECEIVER_H__
#define __ONLINE_DATA_RECEIVER_H__

#include <vector>
#include <set>

#include <iceoryx_posh/runtime/posh_runtime.hpp>
#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>
#if defined (__cplusplus)
extern "C" {
#endif
#include <iceoryx_binding_c/chunk.h>
#include <iceoryx_binding_c/subscriber.h>
#if defined (__cplusplus)
}
#endif

#include "include/decode_event.h"
#include "include/daq_packet.h"


struct GroupPolicyInfo {
	uint32_t size;
	uint32_t valid_packets;
	uint64_t expect_id;
};


class OnlineDataReceiver {
public:
	OnlineDataReceiver(
		const char *app_name,
		const char *service_name
	);

	std::vector<DecodeEvent>* ReceiveEvent(
		const int64_t window
	);


	bool Alive() const;


	void SearchDecodeOffset(size_t mod);

private:

	void Decode(
		const unsigned int *data,
		size_t &offset,
		const int rate,
		DecodeEvent &event
	);

	// crate id
	int crate_;
	// run number
	int run_;
    // number of modules
	size_t module_num_;
	// sampling ratae
	std::vector<int> sampling_rate_;
	// subscriber storage
	iox_sub_storage_t subscriber_storage_[16];
	// subscriber
	iox_sub_t subscriber_[16];
	// user payload
	const void *user_payload_[16];
	// packet header
	const PacketHeader *header_[16];
	// actual packet data
	const DaqPacket *packet_[16];
	// module slot (slot id)
	std::vector<int> module_slot_;
	// has taken chunks
	std::vector<int> has_taken_;

	// group index of each module
	std::vector<int> group_index_;
	// valid group index
	std::set<int> valid_group_index_;
	// group policy information
	GroupPolicyInfo group_info_[16];
	// last decoded group packet id
	int last_packet_id_[16];

	// the first event of each module
	DecodeEvent first_events_[16];

	// received event
	std::vector<DecodeEvent> event_;

	// decode offset
	size_t decode_offset_[16];
};

#endif // __ONLINE_DATA_RECEIVER_H__