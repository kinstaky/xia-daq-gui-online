#pragma once

#include <chrono>
#include <random>

#include "examples/pixie_simulation/alpha_source.h"
#include "examples/pixie_simulation/detector.h"

namespace fake {

struct ModuleInfo {
	unsigned short revision;
	unsigned int serial;
	unsigned short bits;
	unsigned short rate;
};

enum class ModuleStatus {
	Invalid = -1,
	Init,
	Ready,
	Running,
};

struct ModuleEvent {
	uint32_t rate;
	uint32_t crate;
	uint32_t module;
	uint32_t slot;
	uint32_t channel;
	uint32_t energy;
	uint64_t timestamp;
	uint32_t cfd;
	uint32_t cfd_source;
};

class FakePixieService {
public:
	FakePixieService(int rate, int fps, int limits = -1);
	~FakePixieService();
	ModuleInfo *GetModuleInfo(int index);
	void MapSlots(unsigned short* PXISlotMap);
	int Boot();
	int Start();
	int Stop();
	int GetStatus(int index) const;
	unsigned int GetDataSize(int index);
	int GetData(int index, unsigned int size, unsigned int *data);
private:
	int module_num_;
	ModuleStatus module_status_[16];
	ModuleInfo module_info_[16];
	unsigned short slots_[16];

	unsigned int data_size_[16];
	unsigned int* module_data_[16];

	AlphaSource source_;
	Detector detector_;
	int limits_;
	int count_;

	bool first_;
	int rate_;
	int fps_;
	std::chrono::time_point<std::chrono::steady_clock> last_;

	std::mt19937 rng_;
	std::uniform_real_distribution<double> interval_distribution_;
	double accumulated_interval_;
	double detect_time_;
	double dead_time_;

	void PrePareEvents();
	ModuleEvent MapDssdEvent(const DetectorEvent &event);
	void FillBinaryData(const ModuleEvent &event);
};

}