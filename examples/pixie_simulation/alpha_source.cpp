#include "examples/pixie_simulation/alpha_source.h"

#include <thread>

namespace fake {

AlphaSource::AlphaSource()
: rng_(std::random_device{}())
, energy_distribution_(0, 2)
, position_distribution_(0.0, 6.0) {}

ParticleEvent AlphaSource::Emit() {
	ParticleEvent event;
	event.energy = alpha_energy[energy_distribution_(rng_)];
	event.tx = position_distribution_(rng_);
	event.ty = position_distribution_(rng_);
	event.tz = 0.0;
	double far_x = position_distribution_(rng_);
	double far_y = position_distribution_(rng_);
	double far_z = -800.0;
	double dx = event.tx - far_x;
	double dy = event.ty - far_y;
	double dz = event.tz - far_z;
	double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
	event.dx = dx / distance;
	event.dy = dy / distance;
	event.dz = dz / distance;
	return event;
}

}