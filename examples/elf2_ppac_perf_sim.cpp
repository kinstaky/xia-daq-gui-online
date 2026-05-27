#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <csignal>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>
#if defined (__cplusplus)
extern "C" {
#endif
#include <iceoryx_binding_c/chunk.h>
#include <iceoryx_binding_c/publisher.h>
#include <iceoryx_binding_c/runtime.h>
#if defined (__cplusplus)
}
#endif

#include "external/pixie/app/pixie16app_defs.h"
#include "include/daq_packet.h"

namespace {

constexpr char kAppName[] = "elf2_ppac_perf_sim";
constexpr char kServiceName[] = "DaqPacket";
constexpr int kRunNumber = 0;
constexpr int kCrateId = 0;
constexpr int kModuleIndex = 0;
constexpr int kModuleSlot = 2;
constexpr int kSamplingRate = 250;
constexpr int kParticleRate = 10000;
constexpr int kParticlesPerTick = 100;
constexpr int kTickHz = kParticleRate / kParticlesPerTick;
constexpr int kPpacChannels = 15;
constexpr size_t kWordsPerEvent = sizeof(DataHeader) / sizeof(unsigned int);
constexpr size_t kPacketPublishThreshold = PACKET_SIZE - EXTFIFO_READ_THRESH;
constexpr double kBeamSigma = 8.0;
constexpr double kAngleSigma = 0.015;
constexpr double kTimingNoiseSigma = 0.35;
constexpr double kAnodeEnergy = 3200.0;
constexpr double kTimingEnergy = 2400.0;
constexpr std::array<double, 3> kPpacZ = {-52.0, -332.0, -612.0};

struct ParticleState {
	double x0;
	double y0;
	double slope_x;
	double slope_y;
	double base_time_ns;
};

struct PublisherState {
	iox_pub_storage_t storage;
	iox_pub_t publisher;
	void *user_payload;
	PacketHeader *header;
	DaqPacket *packet;
	uint64_t packet_id;
	uint64_t published_packets;
	uint64_t allocation_failures;
};

void SaveOnlineInformation() {
	std::string path = std::string(getenv("HOME")) + "/.xia-daq-gui-online";
#if __cplusplus >= 201703L
	std::filesystem::create_directories(path);
#else
	std::experimental::filesystem::create_directories(path);
#endif
	std::ofstream fout(path + "/online_information.txt");
	fout << kRunNumber << "\n"
		<< kCrateId << "\n"
		<< 1 << "\n"
		<< kSamplingRate << "\n"
		<< 0 << "\n"
		<< kModuleSlot << "\n";
}

void AllocatePayload(PublisherState &state) {
	if (state.user_payload) return;
	const uint32_t alignment = 8;
	enum iox_AllocationResult res = iox_pub_loan_aligned_chunk_with_user_header(
		state.publisher,
		&state.user_payload,
		sizeof(DaqPacket),
		alignment,
		sizeof(PacketHeader),
		alignment
	);
	if (res != AllocationResult_SUCCESS) {
		++state.allocation_failures;
		state.user_payload = nullptr;
		state.header = nullptr;
		state.packet = nullptr;
		return;
	}
	state.header = static_cast<PacketHeader*>(iox_chunk_header_to_user_header(
		iox_chunk_header_from_user_payload(state.user_payload)
	));
	state.header->length = 0;
	state.packet = static_cast<DaqPacket*>(state.user_payload);
}

void PublishCurrentPacket(PublisherState &state) {
	if (!state.user_payload || !state.header || state.header->length == 0) return;
	state.header->id = state.packet_id++;
	iox_pub_publish_chunk(state.publisher, state.user_payload);
	state.user_payload = nullptr;
	state.header = nullptr;
	state.packet = nullptr;
	++state.published_packets;
}

uint32_t EncodeCfd250(double cfd_ns) {
	double clamped = std::max(-3.999, std::min(3.999, cfd_ns));
	unsigned int cfds = clamped < 0.0 ? 1U : 0U;
	double scaled = (clamped / 4.0 + static_cast<double>(cfds)) * 16384.0;
	unsigned int field = static_cast<unsigned int>(std::lround(scaled));
	if (field > 0x3fffU) field = 0x3fffU;
	return (cfds << 14) | field;
}

void FillBinaryEvent(
	DataHeader &header,
	int channel,
	uint64_t timestamp_ticks,
	double cfd_ns,
	uint32_t energy
) {
	std::fill(std::begin(header.data), std::end(header.data), 0U);
	header.data[0] |= channel & 0xf;
	header.data[0] |= (kModuleSlot & 0xf) << 4;
	header.data[0] |= (kCrateId & 0xf) << 8;
	header.data[0] |= 4U << 12;
	header.data[0] |= 4U << 17;
	header.data[1] = static_cast<uint32_t>(timestamp_ticks & 0xffff'ffffULL);
	header.data[2] |= static_cast<uint32_t>((timestamp_ticks >> 32) & 0xffffULL);
	header.data[2] |= EncodeCfd250(cfd_ns) << 16;
	if (energy > 65535U) energy = 65535U;
	header.data[3] |= energy & 0xffffU;
}

void AppendEvent(PublisherState &state, const DataHeader &event) {
	AllocatePayload(state);
	if (!state.user_payload) return;
	if (state.header->length + kWordsPerEvent > kPacketPublishThreshold) {
		PublishCurrentPacket(state);
		AllocatePayload(state);
		if (!state.user_payload) return;
	}
	memcpy(
		state.packet->data + state.header->length,
		&event,
		sizeof(DataHeader)
	);
	state.header->length += kWordsPerEvent;
}

double SampleTime(
	std::mt19937_64 &generator,
	std::normal_distribution<double> &noise,
	double base_time_ns,
	double position_mm,
	double axis_offset_ns
) {
	return base_time_ns + axis_offset_ns + 2.0 * position_mm + noise(generator);
}

void EmitPpac(
	PublisherState &state,
	std::mt19937_64 &generator,
	std::normal_distribution<double> &timing_noise,
	const ParticleState &particle
) {
	static constexpr std::array<int, 15> channels = {
		0, 1, 2, 3, 12,
		4, 5, 6, 7, 13,
		8, 9, 10, 11, 14
	};

	std::array<double, kPpacChannels> hit_times{};
	std::array<uint32_t, kPpacChannels> energies{};
	for (size_t plane = 0; plane < kPpacZ.size(); ++plane) {
		double z = kPpacZ[plane];
		double x = particle.x0 + particle.slope_x * z;
		double y = particle.y0 + particle.slope_y * z;
		size_t base = plane * 5;
		hit_times[base + 0] = SampleTime(
			generator, timing_noise, particle.base_time_ns, x, 100.0
		);
		hit_times[base + 1] = SampleTime(
			generator, timing_noise, particle.base_time_ns, -x, 100.0
		);
		hit_times[base + 2] = SampleTime(
			generator, timing_noise, particle.base_time_ns, y, 120.0
		);
		hit_times[base + 3] = SampleTime(
			generator, timing_noise, particle.base_time_ns, -y, 120.0
		);
		hit_times[base + 4] = particle.base_time_ns + 110.0 + timing_noise(generator);
		energies[base + 0] = kTimingEnergy;
		energies[base + 1] = kTimingEnergy;
		energies[base + 2] = kTimingEnergy;
		energies[base + 3] = kTimingEnergy;
		energies[base + 4] = kAnodeEnergy;
	}

	for (size_t i = 0; i < channels.size(); ++i) {
		double time_ns = hit_times[i];
		int64_t tick = static_cast<int64_t>(std::llround(time_ns / 8.0));
		double cfd_ns = time_ns - 8.0 * static_cast<double>(tick);
		uint64_t timestamp_ticks = static_cast<uint64_t>(tick);
		DataHeader event;
		FillBinaryEvent(event, channels[i], timestamp_ticks, cfd_ns, energies[i]);
		AppendEvent(state, event);
	}
}

} // namespace

int main() {
	if (kParticleRate % kParticlesPerTick != 0) {
		std::cerr << "[Error] Invalid tick configuration.\n";
		return 1;
	}

	iox_runtime_init(kAppName);
	SaveOnlineInformation();

	iox_pub_options_t options;
	iox_pub_options_init(&options);
	options.historyCapacity = 5U;
	options.subscriberTooSlowPolicy =
		iox_ConsumerTooSlowPolicy::ConsumerTooSlowPolicy_DISCARD_OLDEST_DATA;
	options.nodeName = "elf2-ppac-perf";

	PublisherState state{};
	std::string run_name = "run" + std::to_string(kRunNumber);
	std::string module_name =
		"c" + std::to_string(kCrateId) + "m" + std::to_string(kModuleIndex);
	state.publisher = iox_pub_init(
		&state.storage,
		kServiceName,
		run_name.c_str(),
		module_name.c_str(),
		&options
	);

	std::mt19937_64 generator(0x50504143ULL);
	std::normal_distribution<double> beam_position(0.0, kBeamSigma);
	std::normal_distribution<double> beam_angle(0.0, kAngleSigma);
	std::normal_distribution<double> timing_noise(0.0, kTimingNoiseSigma);

	uint64_t particle_count = 0;
	double time_per_particle_ns = 1.0e9 / static_cast<double>(kParticleRate);
	auto next_tick = std::chrono::steady_clock::now();
	auto last_report = next_tick;

	while (!iox::posix::hasTerminationRequested()) {
		for (int i = 0; i < kParticlesPerTick; ++i) {
			ParticleState particle{
				beam_position(generator),
				beam_position(generator),
				beam_angle(generator),
				beam_angle(generator),
				static_cast<double>(particle_count) * time_per_particle_ns
			};
			EmitPpac(state, generator, timing_noise, particle);
			++particle_count;
		}

		next_tick += std::chrono::milliseconds(1000 / kTickHz);
		std::this_thread::sleep_until(next_tick);

		auto now = std::chrono::steady_clock::now();
		if (now - last_report >= std::chrono::seconds(1)) {
			last_report = now;
			std::cout
				<< "[elf2_ppac_perf_sim] particles=" << particle_count
				<< " packets=" << state.published_packets
				<< " alloc_failures=" << state.allocation_failures
				<< " current_words=" << (state.header ? state.header->length : 0)
				<< "\n";
		}
	}

	PublishCurrentPacket(state);
	iox_pub_deinit(state.publisher);

	return 0;
}
