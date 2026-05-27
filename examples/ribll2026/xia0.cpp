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
#include "examples/alpha_source_dssd_global.h"

// GUI fresh rate(FPS), in Hz
constexpr int fresh_rate = 10;
// total number of graphs in online
constexpr int graph_num = 3;
constexpr int graph_num2 = 2;
// window width in pixel
int window_width = 1200;
// window height in pixel
int window_height = 600;
// correlation window, in nanoseconds
int64_t time_window = 10000;
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
	TCanvas *canvas2,
	SignalHandler *handler,
	SignalHandler *handler2,
	const std::vector<TH1*> &histograms,
	const std::string &screenshot_name
) {
	while (!iox::posix::hasTerminationRequested()) {
		for (int i = 1; i <= graph_num; ++i) {
			if(i<2) canvas->cd(i);
			if(i==2) canvas->cd(4);
			if(i==3) canvas->cd(6);
			canvas->Update();
			canvas->Pad()->Draw();
		}
		for (int i = 1; i <= graph_num2; ++i) {
                        canvas2->Update();
                        canvas2->Pad()->Draw();
                }

		if (handler->ShouldRefresh() || handler2->ShouldRefresh()) {
			for (size_t i = 0; i < histograms.size(); ++i) {
				histograms[i]->Reset();
			}
		}
		if (handler->ShouldSave() || handler2->ShouldSave()) {
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
	TH2F &hist_front_strip_energy,
	TH2F &hist_back_strip_energy,
	TH2F &hist_detof,
	TH1F &hist_tof,
	double &current_time
) {
	// can't find enough events
	// if (decode_event.size() != 2) return;

	int front_strip[5],frontnum=0;
	int back_strip[5],backnum=0;
	double front_energy[5];
	double back_energy[5];
	double front_time[5];
	double back_time[5];
	double t1plastict,t2plastict,t2sie;

	for (const DecodeEvent &event : decode_event) {
		if (event.module == 0 && frontnum < 5 && backnum < 5) {
			switch(event.channel){
				case 0: t1plastict = event.time; break;//plastic1
				case 1: t2plastict = event.time; break;//plastic2
				case 2: t2sie = event.energy; break;//si

				case 14: if(event.energy<300) {break;}
					     front_strip[frontnum] = 15;
					     front_energy[frontnum] = double(event.energy);
                         front_time[frontnum] = event.time;
						 frontnum++;
					     break;
					 
				case 15:   if(event.energy<300) {break;}
					     back_strip[backnum] = 16;
                         back_energy[backnum] = double(event.energy);
                         back_time[backnum] = event.time;
						 backnum++;
                         break;
					
            }
		}

		if (event.module > 0  && event.module < 5 && frontnum < 5 && event.energy>300) {
			front_strip[frontnum] = (event.module-1) * 16 + event.channel;
			front_energy[frontnum] = double(event.energy);
			front_time[frontnum] = event.time;
			frontnum++;
			// double cali_p0 = calibration_parameters[front_strip][1];
			// double cali_p1 = calibration_parameters[front_strip][0];
			// front_energy = cali_p0 + cali_p1 * front_energy;
		}
		else if (event.module > 4 && event.module < 9 && backnum < 5 && event.energy>300) {
			back_strip[backnum] = (event.module-5) * 16 + event.channel;
            back_energy[backnum] = double(event.energy);
            back_time[backnum] = event.time;
			backnum++;
            // double cali_p0 = calibration_parameters[back_strip+32][1];
            // double cali_p1 = calibration_parameters[back_strip+32][0];
            // back_energy = cali_p0 + cali_p1 * back_energy;
		}
	}

	// No valid front/back strip was collected for this event group.
	if (frontnum <= 0 || backnum <= 0) return;

	// check front-back energy correlation
	// if (fabs(front_energy - back_energy) > 3000) return;

	for(int i=0;i<frontnum && i<backnum;i++){
		hist_strip.Fill(63-back_strip[i],63-front_strip[i]);
		hist_front_strip_energy.Fill(front_strip[i],front_energy[i]);
		hist_back_strip_energy.Fill(back_strip[i],back_energy[i]);
	}

	if(t1plastict>0 && t2plastict>0){
		hist_detof.Fill(t2plastict-t1plastict,t2sie);
		hist_tof.Fill(t2plastict-t1plastict);		
	}

	current_time = front_time[0];
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
	TCanvas* canvas = new TCanvas("canvas", "Online", 0, 0, 600, 900);
	TCanvas* canvas2 = new TCanvas("canvas2", "Online", 0, 0, 600, 600);
	// create histograms
	TH2F hist_strip("hs", "Pixel on DSSD2", 64, 0, 64, 64, 0, 64);
	TH2F hist_detof("he", "dE-ToF", 1000, -200, 800, 1000, 0, 66000);
	TH2F hist_front_strip_energy("hfse","energy of front strips",64,0,64,1000,0,10000);
	TH2F hist_back_strip_energy("hbse","energy of back strips",64,0,64,1000,0,10000);
	TH1F hist_tof("htof","ToF",1000,-200,800);
	// draw
	canvas->Divide(2,3);
	canvas->cd(1);
	hist_strip.Draw("colz");
	//canvas->cd(2);
	//hist_detof.Draw("colz");
	canvas->cd(4);
	gPad->SetPad(0.0,0.34,1.0,0.67);
	hist_front_strip_energy.Draw("colz");
	canvas->cd(6);
	gPad->SetPad(0.0,0.0,1.0,0.34);
	hist_back_strip_energy.Draw("colz");

	canvas2->Divide(1,2);
	canvas2->cd(1);
	hist_detof.Draw("colz");
	canvas2->cd(2);
	hist_tof.Draw("");

	// handle signal
	std::unique_ptr<SignalHandler> signal_handler = HandleSignal(canvas);
	std::unique_ptr<SignalHandler> signal_handler2 = HandleSignal(canvas2);
	// histograms
	std::vector<TH1*> histograms = {
		&hist_strip,
		&hist_front_strip_energy,
		&hist_back_strip_energy,
		&hist_detof,
		&hist_tof,
	};
	std::string screenshot_name = GetScreenShotName();
	// update GUI
	std::thread update_gui_thread(
		UpdateGui,
		canvas,
		canvas2,
		signal_handler.get(),
		signal_handler2.get(),
		histograms,
		screenshot_name
	);

	OnlineDataReceiver receiver(app_name, "DaqPacket");
	// OnlineDataReceiver receiver(app_name, "ExampleSimulateOnline");
	// time of last event
	double last_time;
	while (receiver.Alive()) {
		for (
			std::vector<DecodeEvent> *event = receiver.ReceiveEvent(time_window);
			event;
			event = receiver.ReceiveEvent(time_window)
		) {
			FillOnlineGraph(
				*event, last_time,
				hist_strip, hist_front_strip_energy,hist_back_strip_energy,hist_detof, hist_tof,
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
