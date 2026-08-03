#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <fstream>

#include <TF1.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TRootCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TSystem.h>
#include <TROOT.h>
#include <TString.h>

#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>

#include "include/signal_handler.h"
#include "include/online_data_receiver.h"
#include "examples/dssd_global.h"

// GUI fresh rate(FPS), in Hz
constexpr int fresh_rate = 10;
// total number of graphs in online
constexpr int graph_num = 5;
// window width in pixel
int window_width = 1200;
// window height in pixel
int window_height = 600;
// correlation window, in nanoseconds
int64_t time_window = 1000;
// screenshot path
const std::string screenshot_path =
	std::string(getenv("HOME")) + "/Pictures/online/";


std::string GetTime() {
	// get current time
	time_t current_time = time(NULL);
	tm* current = localtime(&current_time);
	// format time
	char time_str[32];
	strftime(time_str, 32, "%Y-%m-%d-%H-%M-%S", current);
	return std::string(time_str);
}

/// @brief update ROOT GUI with specific FPS, read keyboard and refresh histograms
/// @param[in] canvas pointer to main ROOT canvas
/// @param[in] handler pointer to signal handler
/// @param[in] histograms vector of pointers to histograms
/// @param[in] screenshot_name name of screenshot file
///
void UpdateGui(
	TCanvas *canvas,
	SignalHandler *handler,
	const std::vector<TH1*> &histograms,
	const std::string &screenshot_name
) {
	while (!iox::posix::hasTerminationRequested()) {
		for (int i = 1; i <= graph_num; ++i) {
			canvas->cd(i);
			canvas->Update();
			canvas->Pad()->Draw();
		}
		if (handler->ShouldRefresh()) {
			for (size_t i = 0; i < histograms.size(); ++i) {
				histograms[i]->Reset();
			}
		}
		if (handler->ShouldSave()) {
			canvas->Print((
				screenshot_path + screenshot_name + GetTime() + ".png"
			).c_str());
		}
		gSystem->ProcessEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/fresh_rate));
	}
	canvas->Print((
		screenshot_path + screenshot_name + GetTime() + ".png"
	).c_str());
}


std::string GetScreenShotName() {
	// get run and crate
	int run, crate;
	// get file name
	std::string file_name =
		std::string(getenv("HOME"))
		+ "/.xia-daq-gui-online/online_information.txt";
	// input file stream
	std::ifstream fin(file_name);
	fin >> run >> crate;
	fin.close();

	TString name;
	name.Form("c%d-r%04d-", crate, run);
	std::string result(name.Data());

	return result;
}


void FillOnlineGraph(
	const std::vector<DecodeEvent> &decode_event,
	double last_time,
	TH2F &hist_strip,
	TH1F &hist_energy,
	TH1F &hist_energy_difference,
	TH1F &hist_interval,
	TH1F &hist_time_difference,
	double &current_time
) {
	// can't find enough events
	if (decode_event.size() != 2) return;

	int front_strip = -1;
	int back_strip = -1;
	double front_energy = -1.0;
	double back_energy = -1.0;
	double front_time = -1.0;
	double back_time = -1.0;
	for (const DecodeEvent &event : decode_event) {
		if (event.module < 2) {
			front_strip = event.module * 16 + event.channel;
			front_energy = double(event.energy);
			front_time = event.time;
			double cali_p0 = calibration_parameters[front_strip][1];
			double cali_p1 = calibration_parameters[front_strip][0];
			front_energy = cali_p0 + cali_p1 * front_energy;
		} else {
			back_strip = (event.module-2) * 16 + event.channel;
            back_energy = double(event.energy);
            back_time = event.time;
            double cali_p0 = calibration_parameters[back_strip+32][1];
            double cali_p1 = calibration_parameters[back_strip+32][0];
            back_energy = cali_p0 + cali_p1 * back_energy;
		}
	}

	// check strips
	if (front_strip < 0 || back_strip < 0) return;

	hist_energy_difference.Fill(front_energy - back_energy);

	// check front-back energy correlation
	if (fabs(front_energy - back_energy) > 0.5) return;

	hist_strip.Fill(front_strip, back_strip);
	hist_energy.Fill(front_energy);
	if (last_time > 0) {
		hist_interval.Fill(front_time - last_time);
	}
	hist_time_difference.Fill(front_time - back_time);
	current_time = front_time;
}


std::unique_ptr<SignalHandler> HandleSignal(TCanvas *canvas) {
	// signal handler
	std::unique_ptr<SignalHandler> signal_handler =
		std::make_unique<SignalHandler>();
	// connect close window and terminate program
	TRootCanvas *rc = (TRootCanvas*)canvas->GetCanvasImp();
	rc->Connect(
		"CloseWindow()",
		"SignalHandler", signal_handler.get(), "Terminate()"
	);
	canvas->Connect(
		"ProcessedEvent(Int_t, Int_t, Int_t, TObject*)",
		"SignalHandler",
		signal_handler.get(),
		"Refresh(Int_t, Int_t, Int_t, TObject*)"
	);
	return signal_handler;
}


int main(int argc, char **argv) {
	constexpr char app_name[] = "online_example";


	// ROOT multi-thread preparation
	ROOT::EnableThreadSafety();
	// create ROOT application
	TApplication app(app_name, &argc, argv);

	// create canvas
	TCanvas* canvas = new TCanvas("canvas", "Online", 0, 0, 1200, 600);
	// create histograms
	TH2F hist_strip("hs", "Pixel on DSSD", 32, 0, 32, 32, 0, 32);
	TH1F hist_energy("he", "energy", 100, 5, 6);
	TH1F hist_energy_difference("hde", "energy difference", 100, -1, 1);
	TH1F hist_interval("hit", "time interval", 1000, 0, 1000000);
	TH1F hist_time_difference("ht", "time difference of two sides", 100, -100, 100);
	// draw
	canvas->Divide(3, 2);
	canvas->cd(1);
	hist_strip.Draw("colz");
	canvas->cd(2);
	hist_energy.Draw();
	canvas->cd(3);
	hist_energy_difference.Draw();
	canvas->cd(4);
	hist_interval.Draw();
	canvas->cd(5);
	hist_time_difference.Draw();


	// handle signal
	std::unique_ptr<SignalHandler> signal_handler = HandleSignal(canvas);
	// histograms
	std::vector<TH1*> histograms = {
		&hist_strip,
		&hist_energy,
		&hist_energy_difference,
        &hist_interval,
        &hist_time_difference
	};
	std::string screenshot_name = GetScreenShotName();
	// update GUI
	std::thread update_gui_thread(
		UpdateGui,
		canvas,
		signal_handler.get(),
		histograms,
		screenshot_name
	);


	OnlineDataReceiver receiver(app_name, "ExampleSimulateOnline");
	// time of last event
	double last_time = -1.0;
	while (receiver.Alive()) {
		for (
			std::vector<DecodeEvent> *event = receiver.ReceiveEvent(time_window);
			event;
			event = receiver.ReceiveEvent(time_window)
		) {
			FillOnlineGraph(
				*event, last_time,
				hist_strip, hist_energy, hist_energy_difference,
				hist_interval, hist_time_difference,
				last_time
			);
		}
	}

	// wait for thread
	update_gui_thread.join();
	// terminate ROOT app
	gApplication->Terminate();

	return 0;
}