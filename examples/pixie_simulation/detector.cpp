#include "examples/pixie_simulation/detector.h"

namespace fake {

Detector::Detector()
: rng_(std::random_device{}())
, dssd_energy_distribution_(0.0, dssd_resolution/(2.0*sqrt(2.0*log(2.0))))
, dssd_time_distribution_(0.0, dssd_time_resolution/(2.0*sqrt(2.0*log(2.0)))) {
}

std::vector<DetectorEvent> Detector::Detect(
	const ParticleEvent &particle,
	const double time
) {
	double lambda = (dssd_distance - particle.tz) / particle.dz;
	double dssd_x = particle.tx + lambda * particle.dx;
	double dssd_y = particle.ty + lambda * particle.dy;
	if (fabs(dssd_x) > dssd_size/2.0 || fabs(dssd_y) > dssd_size/2.0) return {};
	DetectorEvent x_event, y_event;
	x_event.index = y_event.index = 3;
	// strips
	x_event.channel = int((dssd_x + dssd_size/2.0) / 2.0);
	y_event.channel = int((dssd_y + dssd_size/2.0) / 2.0) + dssd_strips;
	// energy
	x_event.energy = particle.energy*(1.0+dssd_energy_distribution_(rng_));
	y_event.energy = particle.energy*(1.0+dssd_energy_distribution_(rng_));
	// time
	x_event.time = time+dssd_time_distribution_(rng_);
	y_event.time = time+dssd_time_distribution_(rng_);

	return {x_event, y_event};
}

}