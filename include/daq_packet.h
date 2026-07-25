#ifndef __DAQ_PACKET_H__
#define __DAQ_PACKET_H__

constexpr size_t PACKET_SIZE = 32768; // 128kB
// constexpr size_t PACKET_SIZE = 4096; // 16kB
//constexpr size_t PACKET_SIZE = 1024; // 4kB

struct DaqPacket {
	unsigned int data[PACKET_SIZE];
};


struct PacketHeader {
	// data length, in words (4 bytes)
	size_t length;
	// sequence ID
	uint64_t id;
};

struct DataHeader {
	unsigned int data[4];
};

struct DataEnergySum {
	unsigned int trailing;
	unsigned int leading;
	unsigned int gap;
	float baseline;
};

struct DataQDC {
	unsigned int sum[8];
};

struct DataExternalTime {
	unsigned int low;
	unsigned int high;
};


#endif // __DAQ_PACKET_H__
