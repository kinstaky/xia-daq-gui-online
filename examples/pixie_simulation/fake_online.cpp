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
#include <TLine.h>

#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>

#include "include/signal_handler.h"
#include "include/online_data_receiver.h"
#include "examples/dssd_global.h"

// GUI fresh rate(FPS), in Hz
constexpr int fresh_rate = 10;
// total number of graphs in online
constexpr int graph_num[2] = {4, 9};
// window width in pixel
int window_width = 1600;
// window height in pixel
int window_height = 1000;
// correlation window, in nanoseconds
int64_t time_window = 1000;
// screenshot path
const std::string screenshot_path =
	std::string(getenv("HOME")) + "/Pictures/online/";

// ppac position
double ppac_xz[3] = {-200.0, -400.0, -600.0};
double ppac_yz[3] = {-200.0, -400.0, -600.0};

void LinearRegression(double *x, double *y, int n, double *par) {
	double sumx = 0.0;
	double sumy = 0.0;
	double sumxy = 0.0;
	double sumx2 = 0.0;
	for (int i = 0; i < n; ++i) {
		sumx += x[i];
		sumy += y[i];
		sumxy += x[i]*y[i];
		sumx2 += x[i]*x[i];
	}
	par[1] = (n*sumxy - sumx*sumy) / (n*sumx2 - sumx*sumx);
	par[0] = sumy/n - sumx/n*par[1];
}

std::string GetTime() {
	// get current time
	time_t current_time = time(NULL);
	tm* current = localtime(&current_time);
	// format time
	char time_str[32];
	strftime(time_str, 32, "%Y-%m-%d-%H-%M-%S", current);
	return std::string(time_str);
}

std::string GetScreenShotName(int index) {
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

	TString name = TString::Format("c%d-r%04d-p%d", crate, run, index);
	std::string result(name.Data());

	return result;
}


/// @brief update ROOT GUI with specific FPS, read keyboard and refresh histograms
/// @param[in] canvas pointer to main ROOT canvas
/// @param[in] handler pointer to signal handler
/// @param[in] histograms vector of pointers to histograms
/// @param[in] screenshot_name name of screenshot file
///
void UpdateGui(
	const std::vector<TCanvas*> &canvas,
	const std::vector<SignalHandler*> &handlers,
	const std::vector<std::vector<TH1*>> &histograms
) {
	while (!iox::posix::hasTerminationRequested()) {
		for (size_t i = 0; i < canvas.size(); ++i) {
			for (int j = 0; j < graph_num[i]; ++j) {
				canvas[i]->cd(j+1);
				canvas[i]->Update();
				canvas[i]->Pad()->Draw();
			}
			if (handlers[i]->ShouldRefresh()) {
				for (size_t j = 0; j < histograms[i].size(); ++j) {
					histograms[i][j]->Reset();
				}
			}
			if (handlers[i]->ShouldSave()) {
				canvas[i]->Print((
					screenshot_path + GetScreenShotName(i) + GetTime() + ".png"
				).c_str());
			}
		}
		gSystem->ProcessEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/fresh_rate));
	}
	for (size_t i = 0; i < canvas.size(); ++i) {
		canvas[i]->Print((
			screenshot_path + GetScreenShotName(i) + GetTime() + ".png"
		).c_str());
	}
}

void FillLine(TH2F &hist, double *par) {
	for (double z = -798.0; z <= 200.0; z += 5.0) {
		hist.Fill(z, par[1]*z + par[0]);
	}
}

void FillOnlineGraph(
	const std::vector<DecodeEvent> &decode_event,
	TH2F &hist_strip,
	TH1F &hist_energy,
	TH2F &hist_front_energy,
	TH2F &hist_back_energy,
	TH2F &hist_ppac1,
	TH2F &hist_ppac2,
	TH2F &hist_ppac3,
	TH2F &hist_target,
	TH2F &hist_xz_trace,
	TH2F &hist_yz_trace
) {
	int front_strip = -1;
	int back_strip = -1;
	double front_channel = -1.0;
	double back_channel = -1.0;
	double front_energy = -1.0;
	double back_energy = -1.0;
	double ppac[15];
	for (int i = 0; i < 15; ++i) ppac[i] = -100.0;
	double ppac_x[3], ppac_y[3];
	int ppac_valid = 0;
	for (const DecodeEvent &event : decode_event) {
		if (event.module == 0) {
			ppac[event.channel] = event.time;
			ppac_valid |= 1 << event.channel;
		} else if (event.module >= 2 && event.module < 4) {
			if (event.energy < front_channel) continue;
			front_strip = (event.module - 2) * 16 + event.channel;
			front_channel = double(event.energy);
			double cali_p0 = calibration_parameters[front_strip][1];
			double cali_p1 = calibration_parameters[front_strip][0];
			front_energy = cali_p0 + cali_p1 * front_channel;
		} else if (event.module >= 4) {
			if (event.energy < back_channel) continue;
			back_strip = (event.module-4) * 16 + event.channel;
            back_channel = double(event.energy);
            double cali_p0 = calibration_parameters[back_strip+32][1];
            double cali_p1 = calibration_parameters[back_strip+32][0];
            back_energy = cali_p0 + cali_p1 * back_channel;
		}
	}

	// check strips
	if (front_strip >= 0 && back_strip >= 0) {
		if (fabs(front_energy - back_energy) < 0.5) {
			hist_strip.Fill(front_strip, back_strip);
			hist_energy.Fill(front_energy);
		}
	}
	if (front_strip >= 0) {
		hist_front_energy.Fill(front_strip, front_channel);
	}
	if (back_strip >= 0) {
		hist_back_energy.Fill(back_strip, back_channel);
	}
	if ((ppac_valid & 0x100f) == 0x100f) {
		ppac_x[0] = (ppac[0] - ppac[1])/4.0;
		ppac_y[0] = (ppac[2] - ppac[3])/4.0;
		hist_ppac1.Fill(ppac_x[0], ppac_y[0]);
	}
	if ((ppac_valid & 0x20f0) == 0x20f0) {
		ppac_x[1] = (ppac[4] - ppac[5])/4.0;
		ppac_y[1] = (ppac[6] - ppac[7])/4.0;
		hist_ppac2.Fill(ppac_x[1], ppac_y[1]);
	}
	if ((ppac_valid & 0x4f00) == 0x4f00) {
		ppac_x[2] = (ppac[8] - ppac[9])/4.0;
		ppac_y[2] = (ppac[10] - ppac[11])/4.0;
		hist_ppac3.Fill(ppac_x[2], ppac_y[2]);
	}
	double xpar[2], ypar[2];
	if ((ppac_valid & 0x7fff) == 0x7fff) {
		LinearRegression(ppac_xz, ppac_x, 3, xpar);
		LinearRegression(ppac_yz, ppac_y, 3, ypar);
		hist_target.Fill(xpar[0], ypar[0]);
		FillLine(hist_xz_trace, xpar);
		FillLine(hist_yz_trace, ypar);
	}
}


SignalHandler* HandleSignal(TCanvas *canvas) {
	// signal handler
	SignalHandler* signal_handler = new SignalHandler();
	// connect close window and terminate program
	TRootCanvas *rc = (TRootCanvas*)canvas->GetCanvasImp();
	rc->Connect(
		"CloseWindow()",
		"SignalHandler", signal_handler, "Terminate()"
	);
	canvas->Connect(
		"ProcessedEvent(Int_t, Int_t, Int_t, TObject*)",
		"SignalHandler",
		signal_handler,
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
	std::vector<TCanvas*> canvas;
	canvas.push_back(new TCanvas("c1", "Online1", 0, 0, 1200, 1200));
	canvas.push_back(new TCanvas("c2", "Online2", 0, 0, 1200, 1200));
	// create histograms
	// DSSD
	TH2F hist_strip("hs", "Pixel on DSSD", 32, 0, 32, 32, 0, 32);
	TH1F hist_energy("he", "energy", 100, 5, 6);
	TH2F hist_front_energy("hfse", "energy of front strips", 32, 0, 32, 1000, 0, 20000);
	TH2F hist_back_energy("hbse", "energy of back strips", 32, 0, 32, 1000, 0, 20000);
	// PPAC
	TH2F hist_ppac1("ppac1", "ppac1", 50, -50, 50, 50, -50, 50);
	TH2F hist_ppac2("ppac2", "ppac2", 50, -50, 50, 50, -50, 50);
	TH2F hist_ppac3("ppac3", "ppac3", 50, -50, 50, 50, -50, 50);
	TH2F hist_target("target", "target", 50, -50, 50, 50, -50, 50);
	TH2F hist_xz_trace("xztrace", "xz trace", 200, -800, 200, 100, -50, 50);
	TH2F hist_yz_trace("yztrace", "yz trace", 200, -800, 200, 100, -50, 50);
	TLine *lppacx[3], *lppacy[3];
	for (int i = 0; i < 3; ++i) {
		lppacx[i] = new TLine(ppac_xz[i], -25.0, ppac_xz[i], 25.0);
		lppacx[i]->SetLineColor(kRed);
		lppacx[i]->SetLineWidth(3);
		lppacy[i] = new TLine(ppac_yz[i], -25.0, ppac_yz[i], 25.0);
		lppacy[i]->SetLineColor(kRed);
		lppacy[i]->SetLineWidth(3);
	}
	TLine *ltarget = new TLine(0.0, -15.0, 0.0, 15.0);
	ltarget->SetLineColor(kBlack);
	ltarget->SetLineWidth(3);

	// draw
	// c1
	canvas[0]->Divide(2, 2);
	canvas[0]->cd(1);
	hist_strip.Draw("colz");
	canvas[0]->cd(2);
	hist_energy.Draw();
	canvas[0]->cd(3);
	hist_front_energy.Draw("colz");
	canvas[0]->cd(4);
	hist_back_energy.Draw("colz");
	// c2
	canvas[1]->Divide(3, 3);
	canvas[1]->cd(1);
	gPad->SetLogz();
	hist_ppac1.Draw("colz");
	canvas[1]->cd(2);
	gPad->SetLogz();
	hist_ppac2.Draw("colz");
	canvas[1]->cd(3);
	gPad->SetLogz();
	hist_ppac3.Draw("colz");
	canvas[1]->cd(5);
	gPad->SetPad(0.0, 0.34, 0.67, 0.67);
	hist_xz_trace.Draw("colz");
	for (int i = 0; i < 3; ++i) lppacx[i]->Draw("same");
	ltarget->Draw("same");
	canvas[1]->cd(6);
	gPad->SetLogz();
	hist_target.Draw("colz");
	canvas[1]->cd(8);
	gPad->SetPad(0.0, 0.0, 0.67, 0.33);
	hist_yz_trace.Draw("colz");
	for (int i = 0; i < 3; ++i) lppacy[i]->Draw("same");
	ltarget->Draw("same");


	// handle signal
	std::vector<SignalHandler*> signal_handlers;
	for (int i = 0 ; i < 2; ++i) {
		signal_handlers.push_back(HandleSignal(canvas[i]));
	}
	// histograms
	std::vector<std::vector<TH1*>> histograms = {
		{
			&hist_strip,
			&hist_energy,
			&hist_front_energy,
			&hist_back_energy
		},
		{
			&hist_ppac1,
			&hist_ppac2,
			&hist_ppac3,
			&hist_target,
			&hist_xz_trace,
			&hist_yz_trace
		}
	};
	// update GUI
	std::thread update_gui_thread(
		UpdateGui,
		canvas,
		signal_handlers,
		histograms
	);


	OnlineDataReceiver receiver(app_name, "DaqPacket");
	// time of last event
	std::vector<DecodeEvent> *event = nullptr;
	while (receiver.Alive()) {
		if (event) {
			// std::cout << "------------------------------------\n"
			// 	<< "Event size " << event->size() << "\n";
			// for (const auto &evt : *event) {
			// 	std::cout << "  " << evt.module << ", " << evt.channel
			// 		<< ", " << evt.timestamp << ", " << evt.energy << "\n";
			// }
			FillOnlineGraph(
				*event,
				hist_strip, hist_energy, hist_front_energy, hist_back_energy,
				hist_ppac1, hist_ppac2, hist_ppac3,
				hist_target, hist_xz_trace, hist_yz_trace
			);
		}
		event = receiver.ReceiveEvent(time_window);
	}

	// wait for thread
	update_gui_thread.join();
	// terminate ROOT app
	gApplication->Terminate();

	return 0;
}