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
constexpr int graph_num = 4;
// window width in pixel
int window_width = 1200;
// window height in pixel
int window_height = 600;
// correlation window, in nanoseconds
int64_t time_window = 1000000; //changed,1000 before
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
			if(i<3) canvas->cd(i);
      if(i==3) canvas->cd(4);
      if(i==4) canvas->cd(6);
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
	TH2F &hist_strip_dssd3,
	TH2F &hist_energy_dssd3,
	TH2F &hist_strip_dssd1,
	TH2F &hist_energy_dssd1,
	double &current_time
) {
	// can't find enough events
	//if (decode_event.size() != 2) return;

	int front_strip_dssd1[5];
	int back_strip_dssd1[5];
	double front_energy_dssd1[5];
	double back_energy_dssd1[5];
	double front_time_dssd1[5];
	double back_time_dssd1[5];
	int dssd1fnum=0,dssd1bnum=0;
	
	int front_strip_dssd3[5];
	int back_strip_dssd3[5];
	double front_energy_dssd3[5];
	double back_energy_dssd3[5];
	double front_time_dssd3[5];
	double back_time_dssd3[5];
	int dssd3fnum=0,dssd3bnum=0;


	for (const DecodeEvent &event : decode_event) {
		// std::cout<<event.module<<"\t"<<event.channel<<"\t"<<event.energy<<"\t"<<event.time<<std::endl;
		if ((event.module == 0 || event.module == 1) && dssd3fnum < 5) {
			front_strip_dssd3[dssd3fnum] = (event.module) * 16 + event.channel;
			front_energy_dssd3[dssd3fnum] = double(event.energy);
			front_time_dssd3[dssd3fnum] = event.time;
			dssd3fnum++;
		}
		else if ((event.module == 2 || event.module == 3) && dssd3bnum < 5) {
			back_strip_dssd3[dssd3bnum] = (event.module-2) * 16 + event.channel;
            back_energy_dssd3[dssd3bnum] = double(event.energy);
            back_time_dssd3[dssd3bnum] = event.time;
			dssd3bnum++;
		}

		if ((event.module == 4 || event.module == 5) && dssd1fnum < 5) {
			front_strip_dssd1[dssd1fnum] = (event.module-4) * 16 + event.channel;
			front_energy_dssd1[dssd1fnum] = double(event.energy);
			front_time_dssd1[dssd1fnum] = event.time;
			dssd1fnum++;
		}
		if(event.module == 6 && event.channel == 1){
					front_strip_dssd1[dssd1fnum] = 15;
          front_energy_dssd1[dssd1fnum] = double(event.energy);
          front_time_dssd1[dssd1fnum] = event.time;
          dssd1fnum++;

		}
		else if ((event.module == 6 || event.module == 7) && dssd1bnum < 5) {
			back_strip_dssd1[dssd1bnum] = (event.module-6) * 16 + event.channel;
            back_energy_dssd1[dssd1bnum] = double(event.energy);
            back_time_dssd1[dssd1bnum] = event.time;
			dssd1bnum++;
		}
	}
	// check strips
	// if (front_strip < 0 || back_strip < 0) return;

	// hist_energy_difference.Fill(front_energy - back_energy);

	// check front-back energy correlation
	// if (fabs(front_energy - back_energy) > 3000) return;
	for(int i=0;i<dssd3bnum && i<dssd3fnum;i++){
		hist_strip_dssd3.Fill(front_strip_dssd3[i], back_strip_dssd3[i]);
		hist_energy_dssd3.Fill(front_strip_dssd3[i],front_energy_dssd3[i]);
		hist_energy_dssd3.Fill(back_strip_dssd3[i]+32,back_energy_dssd3[i]);
	}

	for(int i=0;i<dssd1bnum && i<dssd1fnum;i++){
		hist_strip_dssd1.Fill(front_strip_dssd1[i], back_strip_dssd1[i]);
		hist_energy_dssd1.Fill(front_strip_dssd1[i],front_energy_dssd1[i]);
		hist_energy_dssd1.Fill(back_strip_dssd1[i]+32,back_energy_dssd1[i]);
	}
	// hist_time_difference.Fill(front_time - back_time);
	current_time = front_time_dssd3[0];
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
	// create histograms
	TH2F hist_strip_dssd3("hs3", "Pixel on DSSD3", 32, 0, 32, 32, 0, 32);
	TH2F hist_strip_energy_dssd3("hse3", "energy of strips on DSSD3", 64, 0, 64, 1000, -1000, 66000);
	TH2F hist_strip_dssd1("hs1", "Pixel on DSSD1", 32, 0, 32, 32, 0, 32);
	TH2F hist_strip_energy_dssd1("hse1", "energy of strips on DSSD1", 64, 0, 64, 1000, -1000, 66000);

	// draw
	canvas->Divide(2, 3);
	canvas->cd(1);
	hist_strip_dssd3.Draw("colz");
	canvas->cd(4);
	gPad->SetPad(0.0,0.34,1.0,0.67);
	hist_strip_energy_dssd3.Draw("colz");
	canvas->cd(2);
	hist_strip_dssd1.Draw("colz");
	canvas->cd(6);
	gPad->SetPad(0.0,0.0,1.0,0.34);
	hist_strip_energy_dssd1.Draw("colz");

	// handle signal
	std::unique_ptr<SignalHandler> signal_handler = HandleSignal(canvas);
	// histograms
	std::vector<TH1*> histograms = {
		&hist_strip_dssd3,
		&hist_strip_energy_dssd3,
		&hist_strip_dssd1,
		&hist_strip_energy_dssd1,
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


	OnlineDataReceiver receiver(app_name, "DaqPacket");
	// time of last event
	double last_time;
	int packnum=0;
	while (receiver.Alive()) {
		for (
			std::vector<DecodeEvent> *event = receiver.ReceiveEvent(time_window);
			event;
			event = receiver.ReceiveEvent(time_window)
		) {
			std::cout<<"recieve package" << packnum<<std::endl;
			FillOnlineGraph(
				*event, last_time,
				hist_strip_dssd3, hist_strip_energy_dssd3,hist_strip_dssd1, hist_strip_energy_dssd1,
				last_time
			);
			std::cout<<"fill package" << packnum<<std::endl;
			packnum++;
//			sleep(1);
		}
	}

	// wait for thread
	update_gui_thread.join();
	// terminate ROOT app
	gApplication->Terminate();

	return 0;
}
