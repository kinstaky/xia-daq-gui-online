#pragma once

#include <cstdint>
#include <random>

#include "examples/pixie_simulation/alpha_source.h"

namespace fake {

// detector settings
// PPAC settings
constexpr double ppac_z[3] = {-200.0, -400.0, -600.0};
constexpr double ppac_size = 50.0;
constexpr int ppac_strips = 51;
// DSSD settings
constexpr double dssd_distance = 10.0;
constexpr double dssd_size = 64.0;
constexpr int dssd_strips = 32;
constexpr double dssd_resolution = 0.01;
constexpr double dssd_time_resolution = 1.2e-9;


struct DetectorEvent {
	int index;
	int channel;
	double energy;
	double time;
};

class Detector {
public:
	Detector();
	std::vector<DetectorEvent> Detect(const ParticleEvent &event, const double time);

private:
	std::mt19937 rng_;
	std::normal_distribution<double> dssd_energy_distribution_;
	std::normal_distribution<double> dssd_time_distribution_;
};



}