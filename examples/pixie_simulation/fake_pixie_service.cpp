#include "examples/pixie_simulation/fake_pixie_service.h"
#include "examples/dssd_global.h"

#include <cstring>
#include <iostream>

namespace fake {

constexpr int kModuleNum = 5;
constexpr ModuleInfo module_info[kModuleNum] = {
	{0xf, 3000, 14, 500},
	{0xf, 1000, 14, 100},
	{0xf, 1001, 14 ,100},
	{0xf, 1002, 14, 100},
	{0xf, 1003, 14, 100}
};

FakePixieService::FakePixieService(int rate, int fps, int limits)
: module_num_(kModuleNum)
, limits_(limits)
, count_(0)
, first_(true)
, rate_(rate)
, fps_(fps)
, rng_(std::random_device{}())
, interval_distribution_(0.0, 1.0)
, accumulated_interval_(0.0)
, detect_time_(0.0)
, dead_time_(1e-6) {
	module_num_ = kModuleNum;
	for (int i = 0; i < module_num_; ++i) {
		module_status_[i] = ModuleStatus::Init;
		module_info_[i] = module_info[i];
		data_size_[i] = 0;
		module_data_[i] = new unsigned int[4096];
		slots_[i] = i+2;
	}
	last_ = std::chrono::steady_clock::now();
}

FakePixieService::~FakePixieService() {
	for (int i = 0; i < module_num_; ++i) {
		delete[] module_data_[i];
	}
}

ModuleInfo *FakePixieService::GetModuleInfo(int index) {
	return module_info_ + index;
}

void FakePixieService::MapSlots(unsigned short* PXISlotMap) {
	for (int i = 0; i < module_num_; ++i) {
		slots_[i] = PXISlotMap[i];
	}
}

int FakePixieService::Boot() {
	for (int i = 0; i < module_num_; ++i) {
		module_status_[i] = ModuleStatus::Ready;
	}
	return 0;
}

int FakePixieService::Start() {
	for (int i = 0; i < module_num_; ++i) {
		module_status_[i] = ModuleStatus::Running;
	}
	count_ = 0;
	detect_time_ = 0.0;
	return 0;
}

int FakePixieService::Stop() {
	for (int i = 0; i < module_num_; ++i) {
		module_status_[i] = ModuleStatus::Ready;
	}
	return 0;
}

int FakePixieService::GetStatus(int index) const {
	if (index < 0 || index > module_num_) return -1;
	if (module_status_[index] == ModuleStatus::Ready) return 0;
	if (module_status_[index] == ModuleStatus::Running) return 1;
	return -1;
}

unsigned int FakePixieService::GetDataSize(int index) {
	if (index < 0 || index >= module_num_) return -1;
	auto now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_);
	if (first_ || duration >= std::chrono::milliseconds(1000/fps_)) {
		first_ = false;
		PrePareEvents();
		last_ = now;
	}
	return data_size_[index];
}

int FakePixieService::GetData(int index, unsigned int size, unsigned int *data) {
	if (index < 0 || index >= module_num_) return -1;
	if (size > data_size_[index]) size = data_size_[index];
	memcpy(data, module_data_[index], size*4);
	memmove(module_data_[index], module_data_[index]+size, (data_size_[index]-size)*4);
	data_size_[index] -= size;
	return 0;
}

void FakePixieService::PrePareEvents() {
	if (limits_ > 0 && count_ >= limits_) return;
	count_++;
	for (int i = 0; i < rate_/fps_; ++i) {
		ParticleEvent event = source_.Emit();
		double interval = -log(interval_distribution_(rng_)) / rate_;
		accumulated_interval_ += interval;
		if (interval >= dead_time_) {
			detect_time_ += accumulated_interval_;
			accumulated_interval_ = 0.0;
		}
		for (auto &event : detector_.Detect(event, detect_time_)) {
			if (event.index < 3) FillBinaryData(MapPpacEvent(event));
			else FillBinaryData(MapDssdEvent(event));
		}
	}
}

ModuleEvent FakePixieService::MapDssdEvent(const DetectorEvent &event) {
	ModuleEvent result;
	result.rate = 100;
	result.crate = 0;
	result.module = event.channel / 16 + 1;
	result.slot = slots_[result.module];
	result.channel = event.channel % 16;
	double p0 = calibration_parameters[event.channel][1];
	double p1 = calibration_parameters[event.channel][0];
	result.energy = (event.energy - p0) / p1;
	result.timestamp = uint64_t(event.time * 1e8);
	result.cfd = uint32_t((event.time*1e8 - double(result.timestamp)) * 32768.0);
	result.cfd_source = 0;
	return result;
}


ModuleEvent FakePixieService::MapPpacEvent(const DetectorEvent &event) {
	ModuleEvent result;
	result.rate = 500;
	result.crate = 0;
	result.module = 0;
	result.slot = 2;
	result.channel = event.channel;
	result.energy = event.energy;
	result.timestamp = uint64_t(event.time * 1e8);
	double t = event.time*1e8 - double(result.timestamp);
	result.cfd_source = uint32_t(t * 5.0);
	result.cfd = uint32_t((t*5.0 - result.cfd_source) * 8192.0);
	return result;
}


void FakePixieService::FillBinaryData(const ModuleEvent &event) {
	const unsigned int &index = event.module;
	unsigned int &size = data_size_[index];
	if (size+4 > 4096) return;
	// std::cout << "module: " << index
	// 	<< " channel: " << event.channel
	// 	<< " energy: " << event.energy
	// 	<< " timestamp: " << event.timestamp << std::endl;
	memset(module_data_[index]+size, 0, 16);
	// channel
	module_data_[index][size] |= event.channel & 0xf;
	// slot
	module_data_[index][size] |= (event.slot & 0xf) << 4;
	// crate
	module_data_[index][size] |= event.crate << 8;
	// header length
	module_data_[index][size] |= 4 << 12;
	// event length
	module_data_[index][size] |= 4 << 17;
	// timemstamp low 32 bits
	module_data_[index][size+1] = uint32_t(event.timestamp & 0xffffffff);
	// timestamp high 16 bits
	module_data_[index][size+2] |= uint32_t((event.timestamp >> 32) & 0xffff);
	// CFD
	if (event.rate == 100) {
		module_data_[index][size+2] |= (event.cfd & 0x7fff) << 16;
	} else if (event.rate == 250) {
		module_data_[index][size+2] |= (event.cfd & 0x3fff) << 16;
		module_data_[index][size+2] |= (event.cfd_source & 0x1) << 30;
	} else {
		module_data_[index][size+2] |= (event.cfd & 0x1fff) << 16;
		module_data_[index][size+2] |= (event.cfd_source & 0x7) << 29;
	}
	// energy
	module_data_[index][size+3] |= event.energy & 0xffff;
	size += 4;
}

}