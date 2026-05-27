#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "external/pixie/app/pixie16app_export.h"
#include "include/daq_packet.h"

namespace {

constexpr char kConfigFile[] = "./parset/cfgPixie16.txt";
constexpr int kDefaultRate = 250;
constexpr int kDefaultBits = 14;
constexpr int kDefaultCrate = 0;
constexpr double kParticleRate = 10000.0;
constexpr double kBeamSigma = 8.0;
constexpr double kAngleSigma = 0.015;
constexpr double kTimingNoiseSigma = 0.35;
constexpr uint32_t kAnodeEnergy = 3200U;
constexpr uint32_t kTimingEnergy = 2400U;
constexpr std::array<double, 3> kPpacZ = {-52.0, -332.0, -612.0};
constexpr std::array<int, 15> kPpacChannels = {
	0, 1, 2, 3, 12,
	4, 5, 6, 7, 13,
	8, 9, 10, 11, 14
};

struct ModuleState {
	unsigned short slot = 2;
	unsigned short bits = kDefaultBits;
	unsigned short rate = kDefaultRate;
	unsigned short rev = 0xf;
	unsigned int serial = 1000;
	bool running = false;
	bool stop_requested = false;
	uint64_t visible_counter = 0;
	uint64_t particles_generated = 0;
	std::deque<unsigned int> fifo_words;
	std::map<std::string, unsigned int> mod_params;
	std::array<std::map<std::string, double>, NUMBER_OF_CHANNELS> chan_params;
};

struct BackendState {
	bool initialized = false;
	unsigned short num_modules = 0;
	unsigned short offline_mode = 0;
	unsigned int crate_id = kDefaultCrate;
	double particle_credit = 0.0;
	uint64_t global_particles = 0;
	std::chrono::steady_clock::time_point last_generate{};
	std::vector<ModuleState> modules;
	std::mt19937_64 generator{0x50504143ULL};
	std::normal_distribution<double> beam_position{0.0, kBeamSigma};
	std::normal_distribution<double> beam_angle{0.0, kAngleSigma};
	std::normal_distribution<double> timing_noise{0.0, kTimingNoiseSigma};
};

BackendState g_backend;
std::mutex g_mutex;

bool ReadScalar(const std::string &key, unsigned int &value) {
	std::ifstream fin(kConfigFile);
	if (!fin.is_open()) return false;
	std::string token;
	while (fin >> token) {
		if (!token.empty() && token[0] == '#') {
			std::string skip;
			std::getline(fin, skip);
			continue;
		}
		if (token == key) {
			fin >> value;
			return true;
		}
	}
	return false;
}

bool ReadVector(const std::string &key, std::vector<unsigned int> &values) {
	std::ifstream fin(kConfigFile);
	if (!fin.is_open()) return false;
	std::string token;
	while (fin >> token) {
		if (!token.empty() && token[0] == '#') {
			std::string skip;
			std::getline(fin, skip);
			continue;
		}
		if (token == key) {
			unsigned int count = 0;
			fin >> count;
			values.clear();
			values.reserve(count);
			for (unsigned int i = 0; i < count; ++i) {
				unsigned int value = 0;
				fin >> value;
				values.push_back(value);
			}
			return true;
		}
	}
	return false;
}

void LoadConfigDefaultsLocked() {
	std::vector<unsigned int> slots;
	std::vector<unsigned int> rates;
	std::vector<unsigned int> bits;
	unsigned int crate = kDefaultCrate;
	ReadScalar("CrateID", crate);
	ReadVector("ModuleSlot", slots);
	ReadVector("ModuleSampingRate", rates);
	ReadVector("ModuleBits", bits);
	g_backend.crate_id = crate;
	for (size_t i = 0; i < g_backend.modules.size(); ++i) {
		ModuleState &module = g_backend.modules[i];
		if (i < slots.size()) module.slot = static_cast<unsigned short>(slots[i]);
		if (i < rates.size()) module.rate = static_cast<unsigned short>(rates[i]);
		if (i < bits.size()) module.bits = static_cast<unsigned short>(bits[i]);
		module.rev = module.bits >= 14 ? 0xf : 0xd;
		module.serial = 1000U + static_cast<unsigned int>(i);
		module.mod_params["CrateID"] = g_backend.crate_id;
		module.mod_params["SlotID"] = module.slot;
		module.mod_params["ModID"] = static_cast<unsigned int>(i);
		module.mod_params["TrigConfig3"] = 0U;
		module.mod_params["SYNCH_WAIT"] = 0U;
		module.mod_params["IN_SYNCH"] = 0U;
	}
}

bool ValidModule(unsigned short mod) {
	return mod < g_backend.modules.size();
}

bool ValidChannel(unsigned short chan) {
	return chan < NUMBER_OF_CHANNELS;
}

uint32_t EncodeCfd250(double cfd_ns) {
	double clamped = std::max(-3.999, std::min(3.999, cfd_ns));
	unsigned int cfds = clamped < 0.0 ? 1U : 0U;
	double scaled = (clamped / 4.0 + static_cast<double>(cfds)) * 16384.0;
	unsigned int field = static_cast<unsigned int>(std::lround(scaled));
	if (field > 0x3fffU) field = 0x3fffU;
	return (cfds << 14) | field;
}

void AppendEventWordsLocked(unsigned short mod, const DataHeader &event) {
	for (unsigned int word : event.data) {
		g_backend.modules[mod].fifo_words.push_back(word);
	}
}

void FillBinaryEvent(
	DataHeader &header,
	unsigned short slot,
	unsigned int crate_id,
	int channel,
	uint64_t timestamp_ticks,
	double cfd_ns,
	uint32_t energy
) {
	std::fill(std::begin(header.data), std::end(header.data), 0U);
	header.data[0] |= channel & 0xf;
	header.data[0] |= (slot & 0xf) << 4;
	header.data[0] |= (crate_id & 0xf) << 8;
	header.data[0] |= 4U << 12;
	header.data[0] |= 4U << 17;
	header.data[1] = static_cast<uint32_t>(timestamp_ticks & 0xffff'ffffULL);
	header.data[2] |= static_cast<uint32_t>((timestamp_ticks >> 32) & 0xffffULL);
	header.data[2] |= EncodeCfd250(cfd_ns) << 16;
	if (energy > 65535U) energy = 65535U;
	header.data[3] = energy;
}

double SampleTimeLocked(double base_time_ns, double position_mm, double axis_offset_ns) {
	return base_time_ns + axis_offset_ns + 2.0 * position_mm + g_backend.timing_noise(g_backend.generator);
}

void GenerateParticlesLocked() {
	bool has_running = false;
	for (const ModuleState &module : g_backend.modules) {
		if (module.running) {
			has_running = true;
			break;
		}
	}
	auto now = std::chrono::steady_clock::now();
	if (g_backend.last_generate.time_since_epoch().count() == 0) {
		g_backend.last_generate = now;
		return;
	}
	if (!has_running) {
		g_backend.last_generate = now;
		return;
	}
	double elapsed = std::chrono::duration<double>(now - g_backend.last_generate).count();
	g_backend.last_generate = now;
	g_backend.particle_credit += elapsed * kParticleRate;
	uint64_t particles = static_cast<uint64_t>(g_backend.particle_credit);
	g_backend.particle_credit -= static_cast<double>(particles);
	if (particles == 0 || g_backend.modules.empty()) return;

	ModuleState &module = g_backend.modules[0];
	double time_per_particle_ns = 1.0e9 / kParticleRate;
	for (uint64_t i = 0; i < particles; ++i) {
		double x0 = g_backend.beam_position(g_backend.generator);
		double y0 = g_backend.beam_position(g_backend.generator);
		double slope_x = g_backend.beam_angle(g_backend.generator);
		double slope_y = g_backend.beam_angle(g_backend.generator);
		double base_time_ns = static_cast<double>(g_backend.global_particles + i) * time_per_particle_ns;
		std::array<double, 15> times{};
		std::array<uint32_t, 15> energies{};
		for (size_t plane = 0; plane < kPpacZ.size(); ++plane) {
			double z = kPpacZ[plane];
			double x = x0 + slope_x * z;
			double y = y0 + slope_y * z;
			size_t base = plane * 5;
			times[base + 0] = SampleTimeLocked(base_time_ns, x, 100.0);
			times[base + 1] = SampleTimeLocked(base_time_ns, -x, 100.0);
			times[base + 2] = SampleTimeLocked(base_time_ns, y, 120.0);
			times[base + 3] = SampleTimeLocked(base_time_ns, -y, 120.0);
			times[base + 4] = base_time_ns + 110.0 + g_backend.timing_noise(g_backend.generator);
			energies[base + 0] = kTimingEnergy;
			energies[base + 1] = kTimingEnergy;
			energies[base + 2] = kTimingEnergy;
			energies[base + 3] = kTimingEnergy;
			energies[base + 4] = kAnodeEnergy;
		}
		for (size_t ch = 0; ch < kPpacChannels.size(); ++ch) {
			double time_ns = times[ch];
			int64_t tick = static_cast<int64_t>(std::llround(time_ns / 8.0));
			double cfd_ns = time_ns - static_cast<double>(tick) * 8.0;
			DataHeader event;
			FillBinaryEvent(
				event,
				module.slot,
				g_backend.crate_id,
				kPpacChannels[ch],
				static_cast<uint64_t>(tick),
				cfd_ns,
				energies[ch]
			);
			AppendEventWordsLocked(0, event);
		}
	}
	g_backend.global_particles += particles;
	module.particles_generated += particles;
}

unsigned int VisibleWordsLocked(unsigned short mod) {
	GenerateParticlesLocked();
	if (!ValidModule(mod)) return 0;
	ModuleState &module = g_backend.modules[mod];
	size_t words = module.fifo_words.size();
	if (words == 0) return 0;
	if (words > 8) {
		size_t trim = static_cast<size_t>(module.visible_counter++ % 4U);
		if (trim < words) words -= trim;
	}
	if (words == 0) words = 1;
	return static_cast<unsigned int>(words);
}

void FillZeros(double *buffer, size_t length) {
	if (!buffer) return;
	for (size_t i = 0; i < length; ++i) buffer[i] = 0.0;
}

void FillZeros(unsigned short *buffer, size_t length) {
	if (!buffer) return;
	memset(buffer, 0, length * sizeof(unsigned short));
}

void FillZeros(unsigned int *buffer, size_t length) {
	if (!buffer) return;
	memset(buffer, 0, length * sizeof(unsigned int));
}

} // namespace

extern "C" {

unsigned int APP32_SetBit(unsigned short bit, unsigned int value) {
	if (bit >= 32) return value;
	return value | (1u << bit);
}

unsigned int APP32_ClrBit(unsigned short bit, unsigned int value) {
	if (bit >= 32) return value;
	return value & ~(1u << bit);
}

unsigned int APP32_TstBit(unsigned short bit, unsigned int value) {
	if (bit >= 32) return 0;
	return (value >> bit) & 0x1u;
}

int Pixie16InitSystem(unsigned short NumModules, unsigned short *PXISlotMap, unsigned short OfflineMode) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (NumModules == 0 || PXISlotMap == nullptr) return -1;
	g_backend = BackendState{};
	g_backend.initialized = true;
	g_backend.num_modules = NumModules;
	g_backend.offline_mode = OfflineMode;
	g_backend.modules.resize(NumModules);
	for (unsigned short i = 0; i < NumModules; ++i) {
		g_backend.modules[i].slot = PXISlotMap[i];
	}
	LoadConfigDefaultsLocked();
	g_backend.last_generate = std::chrono::steady_clock::now();
	return 0;
}

int Pixie16ExitSystem(unsigned short) {
	std::lock_guard<std::mutex> lock(g_mutex);
	g_backend = BackendState{};
	return 0;
}

int Pixie16SetOfflineVariant(unsigned short ModuleNumber, unsigned short variant) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModuleNumber)) return -1;
	g_backend.modules[ModuleNumber].mod_params["OfflineVariant"] = variant;
	return 0;
}

int Pixie16ReadModuleInfo(
	unsigned short ModNum,
	unsigned short *ModRev,
	unsigned int *ModSerNum,
	unsigned short *ModADCBits,
	unsigned short *ModADCMSPS
) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	const ModuleState &module = g_backend.modules[ModNum];
	if (ModRev) *ModRev = module.rev;
	if (ModSerNum) *ModSerNum = module.serial;
	if (ModADCBits) *ModADCBits = module.bits;
	if (ModADCMSPS) *ModADCMSPS = module.rate;
	return 0;
}

int Pixie16BootModule(
	const char *,
	const char *,
	const char *,
	const char *,
	const char *,
	const char *,
	unsigned short ModNum,
	unsigned short
) {
	std::lock_guard<std::mutex> lock(g_mutex);
	return ValidModule(ModNum) ? 0 : -1;
}

int Pixie16ReadSglModPar(const char *ModParName, unsigned int *ModParData, unsigned short ModNum) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	if (!ModParData) return -2;
	auto &params = g_backend.modules[ModNum].mod_params;
	auto it = params.find(ModParName ? ModParName : "");
	*ModParData = it == params.end() ? 0U : it->second;
	return 0;
}

int Pixie16WriteSglModPar(const char *ModParName, unsigned int ModParData, unsigned short ModNum) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	std::string key = ModParName ? ModParName : "";
	g_backend.modules[ModNum].mod_params[key] = ModParData;
	if (key == "CrateID") g_backend.crate_id = ModParData;
	return 0;
}

int Pixie16ReadSglChanPar(
	const char *ChanParName,
	double *ChanParData,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	if (!ValidChannel(ChanNum)) return -2;
	if (!ChanParData) return -3;
	auto &params = g_backend.modules[ModNum].chan_params[ChanNum];
	auto it = params.find(ChanParName ? ChanParName : "");
	*ChanParData = it == params.end() ? 0.0 : it->second;
	return 0;
}

int Pixie16WriteSglChanPar(
	const char *ChanParName,
	double ChanParData,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	if (!ValidChannel(ChanNum)) return -2;
	g_backend.modules[ModNum].chan_params[ChanNum][ChanParName ? ChanParName : ""] = ChanParData;
	return 0;
}

int Pixie16AdjustOffsets(unsigned short ModNum) {
	return ValidModule(ModNum) ? 0 : -1;
}

int Pixie16BLcutFinder(unsigned short ModNum, unsigned short ChanNum, unsigned int *BLcut) {
	if (!ValidModule(ModNum)) return -1;
	if (!ValidChannel(ChanNum)) return -1;
	if (BLcut) *BLcut = 0U;
	return 0;
}

int Pixie16ReadCSR(unsigned short ModNum, unsigned int *CSR) {
	if (!ValidModule(ModNum)) return -1;
	if (CSR) *CSR = 0U;
	return 0;
}

int Pixie16StartListModeRun(unsigned short ModNum, unsigned short, unsigned short) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!g_backend.initialized) return -1;
	if (ModNum == g_backend.num_modules) {
		for (ModuleState &module : g_backend.modules) {
			module.running = true;
			module.stop_requested = false;
		}
	} else if (ValidModule(ModNum)) {
		g_backend.modules[ModNum].running = true;
		g_backend.modules[ModNum].stop_requested = false;
	} else {
		return -1;
	}
	g_backend.last_generate = std::chrono::steady_clock::now();
	return 0;
}

int Pixie16CheckExternalFIFOStatus(unsigned int *nFIFOWords, unsigned short ModNum) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	if (!nFIFOWords) return -1;
	*nFIFOWords = VisibleWordsLocked(ModNum);
	return 0;
}

int Pixie16ReadDataFromExternalFIFO(unsigned int *ExtFIFO_Data, unsigned int nFIFOWords, unsigned short ModNum) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	if (!ExtFIFO_Data) return -2;
	GenerateParticlesLocked();
	ModuleState &module = g_backend.modules[ModNum];
	unsigned int words = std::min<unsigned int>(nFIFOWords, static_cast<unsigned int>(module.fifo_words.size()));
	for (unsigned int i = 0; i < words; ++i) {
		ExtFIFO_Data[i] = module.fifo_words.front();
		module.fifo_words.pop_front();
	}
	if (module.stop_requested && module.fifo_words.empty()) {
		module.running = false;
	}
	return 0;
}

int Pixie16CheckRunStatus(unsigned short ModNum) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	const ModuleState &module = g_backend.modules[ModNum];
	return (module.running || (module.stop_requested && !module.fifo_words.empty())) ? 1 : 0;
}

int Pixie16EndRun(unsigned short ModNum) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	GenerateParticlesLocked();
	for (ModuleState &module : g_backend.modules) {
		module.running = false;
		module.stop_requested = true;
	}
	return 0;
}

int Pixie16ReadStatisticsFromModule(unsigned int *Statistics, unsigned short ModNum) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (!ValidModule(ModNum)) return -1;
	if (!Statistics) return -2;
	FillZeros(Statistics, 448);
	Statistics[0] = static_cast<unsigned int>(g_backend.modules[ModNum].particles_generated & 0xffff'ffffULL);
	Statistics[1] = static_cast<unsigned int>(g_backend.modules[ModNum].fifo_words.size());
	return 0;
}

int Pixie16EMbufferIO(
	unsigned int *Buffer,
	unsigned int NumWords,
	unsigned int,
	unsigned short Direction,
	unsigned short ModNum
) {
	if (!ValidModule(ModNum)) return -5;
	if (!Buffer) return -1;
	if (Direction == MOD_READ) FillZeros(Buffer, NumWords);
	return 0;
}

int Pixie16AcquireADCTrace(unsigned short ModNum) {
	return ValidModule(ModNum) ? 0 : -1;
}

int Pixie16ReadSglChanADCTrace(
	unsigned short *Trace_Buffer,
	unsigned int Trace_Length,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	if (!ValidModule(ModNum)) return -1;
	if (!ValidChannel(ChanNum)) return -2;
	FillZeros(Trace_Buffer, Trace_Length);
	return 0;
}

int Pixie16AcquireBaselines(unsigned short ModNum) {
	return ValidModule(ModNum) ? 0 : -1;
}

int Pixie16ReadSglChanBaselines(
	double *Baselines,
	double *TimeStamps,
	unsigned short NumBases,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	if (!ValidModule(ModNum)) return -1;
	if (!ValidChannel(ChanNum)) return -2;
	FillZeros(Baselines, NumBases);
	FillZeros(TimeStamps, NumBases);
	return 0;
}

int Pixie16TauFinder(unsigned short ModNum, double *Tau) {
	if (!ValidModule(ModNum)) return -1;
	if (Tau) {
		for (size_t i = 0; i < NUMBER_OF_CHANNELS; ++i) Tau[i] = 1.0;
	}
	return 0;
}

int Pixie16SaveHistogramToFile(const char *FileName, unsigned short ModNum) {
	if (!ValidModule(ModNum)) return -1;
	if (!FileName) return -4;
	std::ofstream fout(FileName, std::ios::binary);
	if (!fout.is_open()) return -4;
	unsigned int zero = 0;
	fout.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
	return 0;
}

int Pixie16SaveDSPParametersToFile(const char *FileName) {
	if (!FileName) return -2;
	std::ofstream fout(FileName, std::ios::binary);
	if (!fout.is_open()) return -2;
	std::array<unsigned int, N_DSP_PAR * PRESET_MAX_MODULES> zeros{};
	fout.write(reinterpret_cast<const char*>(zeros.data()), sizeof(unsigned int) * zeros.size());
	return 0;
}

int Pixie16CopyDSPParameters(unsigned short, unsigned short, unsigned short, unsigned short *) {
	return 0;
}

int Pixie16complexFFT(double *, unsigned int) {
	return 0;
}

int Pixie16ComputeFastFiltersOffline(
	const char *,
	unsigned short,
	unsigned short,
	unsigned int,
	unsigned short RcdTraceLength,
	unsigned short *,
	double *fastfilter,
	double *cfd
) {
	FillZeros(fastfilter, RcdTraceLength);
	FillZeros(cfd, RcdTraceLength);
	return 0;
}

int Pixie16ComputeSlowFiltersOffline(
	const char *,
	unsigned short,
	unsigned short,
	unsigned int,
	unsigned short RcdTraceLength,
	unsigned short *,
	double *slowfilter
) {
	FillZeros(slowfilter, RcdTraceLength);
	return 0;
}

int HongyiWuPixie16ComputeCFDFiltersOffline(
	unsigned short RcdTraceLength,
	double,
	unsigned short,
	unsigned short,
	unsigned short,
	unsigned short *,
	double *cfd
) {
	FillZeros(cfd, RcdTraceLength);
	return 0;
}

int HongyiWuPixie16ComputeCFDOffline(
	unsigned short,
	double *,
	unsigned short,
	unsigned short,
	short *pointcfd,
	double *cfd
) {
	if (pointcfd) *pointcfd = 0;
	if (cfd) *cfd = 0.0;
	return 0;
}

int HongyiWuPixie16ComputeFastFiltersOffline(
	char *,
	unsigned short,
	unsigned short,
	unsigned int,
	unsigned short RcdTraceLength,
	unsigned short *,
	double *fastfilter,
	double *cfd,
	double *cfds
) {
	FillZeros(fastfilter, RcdTraceLength);
	FillZeros(cfd, RcdTraceLength);
	FillZeros(cfds, RcdTraceLength);
	return 0;
}

int HongyiWuPixie16ComputeSlowFiltersOfflineAverageBaseline(
	char *,
	unsigned short,
	unsigned short,
	unsigned int,
	unsigned short RcdTraceLength,
	unsigned short *,
	double *slowfilter,
	int
) {
	FillZeros(slowfilter, RcdTraceLength);
	return 0;
}

int HongyiWuPixie16ComputeSlowFiltersOfflineExtendBaseline(
	char *,
	unsigned short,
	unsigned short,
	unsigned int,
	unsigned short RcdTraceLength,
	unsigned short *,
	double *slowfilter,
	unsigned int,
	double,
	double,
	double,
	int,
	int
) {
	FillZeros(slowfilter, RcdTraceLength);
	return 0;
}

} // extern "C"
