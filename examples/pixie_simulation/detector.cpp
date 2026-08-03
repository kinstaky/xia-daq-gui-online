#include "examples/pixie_simulation/detector.h"

namespace fake {

Detector::Detector()
: rng_(std::random_device{}())
, ppac_time_distribution_(0.0, 0.35)
, dssd_energy_distribution_(0.0, dssd_resolution/(2.0*sqrt(2.0*log(2.0))))
, dssd_time_distribution_(0.0, dssd_time_resolution/(2.0*sqrt(2.0*log(2.0)))) {
}

std::vector<DetectorEvent> Detector::Detect(
	const ParticleEvent &particle,
	const double time
) {
	std::vector<DetectorEvent> result;

	double ppac_lambda[3] = {
		(ppac_z[0] - particle.tz) / particle.dz,
		(ppac_z[1] - particle.tz) / particle.dz,
		(ppac_z[2] - particle.tz) / particle.dz
	};
	for (int i = 0; i < 3; ++i) {
		double ppac_x = particle.tx + ppac_lambda[i] * particle.dx;
		double ppac_y = particle.ty + ppac_lambda[i] * particle.dy;
		if (fabs(ppac_x) > ppac_size/2.0 || fabs(ppac_y) > ppac_size/2.0) continue;
		DetectorEvent event;
		event.index = i;
		event.energy = 0.0;

		int x_strip = int(ppac_x + 25.5);
		double xdiff = x_strip * 4.0 - 100.0;
		double xsum = 100.0;
		double x1 = (xsum+xdiff)/2.0 + ppac_time_distribution_(rng_);
		double x2 = (xsum-xdiff)/2.0 + ppac_time_distribution_(rng_);
		event.channel = i*4;
		event.time = time + x1*1e-9;
		result.push_back(event);
		event.channel = i*4+1;
		event.time = time + x2*1e-9;
		result.push_back(event);

		int y_strip = int((ppac_y + ppac_size/2.0));
		double ydiff = y_strip * 4.0 - 100.0;
		double ysum = 100.0;
		double y1 = (ysum+ydiff)/2.0 + ppac_time_distribution_(rng_);
		double y2 = (ysum-ydiff)/2.0 + ppac_time_distribution_(rng_);
		event.channel = i*4+2;
		event.time = time + y1*1e-9;
		result.push_back(event);
		event.channel = i*4+3;
		event.time = time + y2*1e-9;
		result.push_back(event);
	}
	// plane event
	for (int i = 0; i < 3; ++i) {
		DetectorEvent event;
		event.index = i;
		event.energy = 0.0;
		event.channel = 12+i;
		event.time = time + ppac_time_distribution_(rng_)*1e-9;
		result.push_back(event);
	}

	double dssd_lambda = (dssd_distance - particle.tz) / particle.dz;
	double dssd_x = particle.tx + dssd_lambda * particle.dx;
	double dssd_y = particle.ty + dssd_lambda * particle.dy;
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
	result.push_back(x_event);
	result.push_back(y_event);

	return result;
}

}