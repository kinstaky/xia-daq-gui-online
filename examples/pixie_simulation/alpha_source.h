#pragma once

#include <random>

namespace fake {

// alpha source settings
// energy of alpha source, in MeV
// 典型 3α 源的能量
constexpr double alpha_energy[3] = {5.157, 5.486, 5.805};

struct ParticleEvent {
	// kinetic energy
	double energy;
	// direction
	double dx, dy, dz;
	// target position
	double tx, ty, tz;
};


class AlphaSource {
public:
	AlphaSource();
	ParticleEvent Emit();
private:
	std::mt19937 rng_;
	std::uniform_int_distribution<int> energy_distribution_;
	std::normal_distribution<double> position_distribution_;
};

}