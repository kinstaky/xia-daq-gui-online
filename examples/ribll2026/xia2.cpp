#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <fstream>

#include <TF1.h>
#include <TLatex.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TRootCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TLine.h>
#include <TSystem.h>
#include <TROOT.h>
#include <TString.h>
#include <TAxis.h>

#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>

#include "include/signal_handler.h"
#include "include/online_data_receiver.h"
#include "examples/alpha_source_dssd_global.h"

// GUI fresh rate(FPS), in Hz
constexpr int fresh_rate = 10;
// total number of graphs in online
constexpr int graph_num = 7;
constexpr int graph_num2 = 4;
// window width in pixel
int window_width = 1200;
// window height in pixel
int window_height = 600;
// correlation window, in nanoseconds
int64_t time_window = 1000; //changed,1000 before
// screenshot path
const std::string screenshot_path =
	std::string(getenv("HOME")) + "/Pictures/online/";

//ppac position
double ppaczxpos[3] = {-52.0,-332.0,-612.0};//mm
double ppaczypos[3] = {-52.0,-332.0,-612.0};//mm
//ppac counts
int64_t ppac_count[9]={0,0,0,0,0,0,0,0,0};//1a,1x,1y,2a,2x,2y,3a,3x,3y

TLatex *line1 = nullptr;
TLatex *line2 = nullptr;
TLatex *line3 = nullptr;

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
			if(i<4) canvas->cd(i);
			else if(i==4) canvas->cd(6);
			else if(i==5) canvas->cd(5);
			else if(i==6) canvas->cd(8);
			else if(i==7){
				canvas->cd(9);
				char ppac1eff[100],ppac2eff[100],ppac3eff[100];
				double efficiency1=0.0;
				double efficiency2=0.0;
				if (ppac_count[0] != 0) {
					efficiency1 = static_cast<double>(ppac_count[1]) / ppac_count[0];
					efficiency2 = static_cast<double>(ppac_count[2]) / ppac_count[0];
				}
				//std::cout<<ppac_count[0]<<"\t"<<efficiency1<<"\t"<<efficiency2<<std::endl;
				snprintf(ppac1eff, sizeof(ppac1eff), "PPAC1 X : %.1f%%, Y : %.1f%%", efficiency1 * 100, efficiency2 * 100);
				line1->SetText(15, 85, ppac1eff);
				efficiency1=0.0;
				efficiency2=0.0;
				if (ppac_count[3] != 0) {
					efficiency1 = static_cast<double>(ppac_count[4]) / ppac_count[3];
					efficiency2 = static_cast<double>(ppac_count[5]) / ppac_count[3];
				}
				snprintf(ppac2eff, sizeof(ppac2eff), "PPAC2 X : %.1f%%, Y : %.1f%%", efficiency1 * 100, efficiency2 * 100);
				line2->SetText(15, 50, ppac2eff);
				efficiency1=0.0;
				efficiency2=0.0;
				if (ppac_count[6] != 0) {
					efficiency1 = static_cast<double>(ppac_count[7]) / ppac_count[6];
					efficiency2 = static_cast<double>(ppac_count[8]) / ppac_count[6];
				}
				
				snprintf(ppac3eff, sizeof(ppac3eff), "PPAC3 X : %.1f%%, Y : %.1f%%", efficiency1 * 100, efficiency2 * 100);
				line3->SetText(15, 15, ppac3eff);
			}
			canvas->Update();
			canvas->Pad()->Draw();
		}
		for (int i = 1; i <= graph_num2; ++i) {
			if(i<3)canvas2->cd(i);
			if(i==3) canvas2->cd(4);
			if(i==4) canvas2->cd(6);
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
			canvas2->Print((
				screenshot_path + screenshot_name + GetTime() + "_2.png"
			).c_str());
		}
		gSystem->ProcessEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/fresh_rate));
	}
	canvas->Print((
		screenshot_path + screenshot_name + GetTime() + ".png"
	).c_str());
	canvas->Print((
		screenshot_path + screenshot_name + GetTime() + "_2.png"
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

void LinearRegression(double *x,double *y,int n,double* par){//calculate trace of beam particles
    double sigmax=0,sigmay=0,sigmaxy=0,sigmax2=0;
    for(Int_t i=0;i<n;i++){
        sigmax = sigmax+x[i];
        sigmay = sigmay+y[i];
        sigmaxy = sigmaxy+x[i]*y[i];
        sigmax2 = sigmax2+x[i]*x[i];
    }
    par[1] = (n*sigmaxy-sigmax*sigmay)/(n*sigmax2-sigmax*sigmax);//k
    par[0] = sigmay/n-sigmax/n*par[1];//b
}

void FillLine(TH2F& h, Double_t b, Double_t k){
    for(Double_t i=-798;i<200;i=i+5){
        h.Fill(i,k*i+b);
    }
    return;
}

void FillOnlineGraph(
	const std::vector<DecodeEvent> &decode_event,
	double last_time,
	TH2F &hist_ppac1,
	TH2F &hist_ppac2,
	TH2F &hist_ppac3,
	TH2F &hist_tarpoint,
	TH1F &hist_hframe,
	TH2F &hist_xztrace,
	TH2F &hist_yztrace,
	TH2F &hist_pos_dssd4,
	TH2F &hist_strip_energy_dssd4,
	TH2F &hist_pos_csi,
	TH2F &hist_strip_energy_csi,
	double &current_time
) {
	// can't find enough events
	// if (decode_event.size() != 2) return;
	// std::cout<<decode_event.size()<<std::endl;
	double ppac1[5] = {-1.0,-1.0,-1.0,-1.0,-1.0};//x1,x2,y1,y2,a
	double ppac2[5] = {-1.0,-1.0,-1.0,-1.0,-1.0};//x1,x2,y1,y2,a
	double ppac3[5] = {-1.0,-1.0,-1.0,-1.0,-1.0}; //x1,x2,y1,y2,a
	double ppacxpos[3] = {-100.0,-100.0,-100.0}, ppacypos[3] = {-100.0,-100.0,-100.0};
	double tarx = -1.0,tary = -1.0;
	double xfitpar[2],yfitpar[2];
	double t1plastict=-1.0,t2plastict=-1.0,t2sie=-1.0;
	//DSSD4
	int front_strip_dssd4[5];
	int back_strip_dssd4[5];
	int frontnum = 0,backnum = 0;
	double front_energy_dssd4[5];
	double back_energy_dssd4[5];
	double front_time_dssd4[5];
	double back_time_dssd4[5];

	//CsI
	int num_csi[5];
	int num_count = 0;;
	int front_num_csi[5];
	int back_num_csi[5];
	double energy_csi[5];

	for (const DecodeEvent &event : decode_event) {
		//if (event.module == 0 &&(event.channel>3 && event.channel<8))std::Icout<<event.module<<"\t"<<event.channel<<"\t"<<event.energy<<"\t"<<event.time<<std::endl;
		//PPAC
		if (event.module == 0 && event.energy>1000) {
			switch(event.channel){
        		        case 0: ppac1[0] = event.time; break;
				case 1: ppac1[1] = event.time; break;
				case 2: ppac1[2] = event.time; break;
				case 3: ppac1[3] = event.time; break;
				case 12: ppac1[4] = event.time; break;
      
	  			case 4: ppac2[0] = event.time; break;
				case 5: ppac2[1] = event.time; break;
				case 6: ppac2[2] = event.time; break;
				case 7: ppac2[3] = event.time; break;
				case 13: ppac2[4] = event.time; break;

				case 8: ppac3[0] = event.time; break;
                                case 9: ppac3[1] = event.time; break;
                                case 10: ppac3[2] = event.time; break;
                                case 11: ppac3[3] = event.time; break;
                                case 14: ppac3[4] = event.time; break;
            }
		}
		if (event.module == 1) {
			switch(event.channel){
        //        case 0: ppac3[0] = event.time; break;
	//			case 1: ppac3[1] = event.time; break;
	//			case 2: ppac3[2] = event.time; break;
	//			case 3: ppac3[3] = event.time; break;
	//			case 4: ppac3[4] = event.time; break;

				case 6: t1plastict = event.time; break;//plastic1
				case 7: t2plastict = event.time; break;//plastic2
				case 8: t2sie = event.energy; break;//si
            }
		}

		//DSSD4
		if((event.module == 2 || event.module == 3) && frontnum<5){
			front_strip_dssd4[frontnum] = (event.module-2) * 16 + event.channel;
			front_energy_dssd4[frontnum] = double(event.energy);
			front_time_dssd4[frontnum] = event.time;
			frontnum++;
		}
		if((event.module == 4 || event.module == 5) && backnum < 5){
			back_strip_dssd4[backnum] = (event.module-4) * 16 + event.channel;
			back_energy_dssd4[backnum] = double(event.energy);
			back_time_dssd4[backnum] = event.time;
			backnum++;
		}

		if(event.module == 7 && event.channel == 7 && frontnum < 5){
			front_strip_dssd4[frontnum] = 16;
			front_energy_dssd4[frontnum] = double(event.energy);
			front_time_dssd4[frontnum] = event.time;
			frontnum++;
		}

        if(event.module == 7 && event.channel >11 && event.channel<16 && frontnum < 5){ 
            front_strip_dssd4[frontnum] = event.channel; 
            front_energy_dssd4[frontnum] = double(event.energy);
            front_time_dssd4[frontnum] = event.time;
            frontnum++;
        }

		//T0 CsI
		if(event.module == 6 && num_count<5){
			if(event.channel != 1){
				num_csi[num_count] = 35 - event.channel;
				if(event.channel <=5) {
					front_num_csi[num_count]  = event.channel;
					back_num_csi[num_count]  = 5;
				}
				else if(event.channel <= 11 && event.channel >=7){
					front_num_csi[num_count]  = 11 - event.channel;
					back_num_csi[num_count]  = 4;
				} 
				else if(event.channel >= 12) {
					front_num_csi[num_count]  = event.channel-12;
					back_num_csi[num_count]  = 3;
				}
				energy_csi[num_count]  = double(event.energy);
				num_count++;
			}
		}

		if(event.module == 7 && num_count<5){
			if(event.channel != 6){
				num_csi[num_count]  = 31 - event.channel;
				if(event.channel <= 1) {
					front_num_csi[num_count]  = event.channel+4;
					back_num_csi[num_count]  = 3;
				}
				else if(event.channel <= 4 && event.channel >= 2){
					front_num_csi[num_count]  = 7 - event.channel;
					back_num_csi[num_count]  = 2;
				} 
                else if(event.channel == 5){
                    front_num_csi[num_count]  = 5;
                    back_num_csi[num_count]  = 4;
                }

				energy_csi[num_count]  = double(event.energy);
				num_count++;
			}
		}

		//if(event.module == 8 && num_count<5){
		//	if(event.channel == 0) {
		//		num_csi[num_count]  = 14;
		//		front_num_csi[num_count]  = 2;
		//		back_num_csi[num_count]  = 2;
		//	}
		//	else if(event.channel <= 4 && event.channel >=1){
		//		num_csi[num_count]  = 36 - event.channel;
		//		front_num_csi[num_count]  = event.channel - 1;
		//		back_num_csi[num_count]  = 5;
		//	} 
		//	else if(event.channel == 5) {
		//		num_csi[num_count]  = 25;
		//		front_num_csi[num_count]  = 1;
		//		back_num_csi[num_count]  = 4;
		//	}
		//	energy_csi[num_count]  = double(event.energy);
		//	}

//            std::cout<<front_strip<<"\t"<<front_energy<<"\t"<<back_strip<<"\t"<<back_energy<<std::endl;
	}
	// ppac pos calculation(mm)
	if (ppac1[0]>0 && ppac1[1]>0 && ppac1[4]>0){
		ppacxpos[0] = (ppac1[0]-ppac1[1])/4;
	}
	if (ppac1[2]>0 && ppac1[3]>0 && ppac1[4]>0){
		ppacypos[0] = (ppac1[2]-ppac1[3])/4;
	}
	if (ppac2[0]>0 && ppac2[1]>0 && ppac2[4]>0){
		ppacxpos[1] = (ppac2[0]-ppac2[1])/4;
	}
	if (ppac2[2]>0 && ppac2[3]>0 && ppac2[4]>0){
		ppacypos[1] = (ppac2[2]-ppac2[3])/4;
	}
	if (ppac3[0]>0 && ppac3[1]>0 && ppac3[4]>0){
		ppacxpos[2] = (ppac3[0]-ppac3[1])/4;
	}
	if (ppac3[2]>0 && ppac3[3]>0 && ppac3[4]>0){
		ppacypos[2] = (ppac3[2]-ppac3[3])/4;
	}
    //std::cout<<ppac1[0]<<"\t" << ppac1[1] << "\t " << ppac1[2] << "\t " << ppac1[3] <<  "\t " << ppac1[4] <<std::endl;
	//std::cout<<ppac2[0]<<"\t" << ppac2[1] << "\t " << ppac2[2] << "\t " << ppac2[3] <<  "\t " << ppac2[4] <<std::endl;
	//std::cout<<ppac3[0]<<"\t" << ppac3[1] << "\t " << ppac3[2] << "\t " << ppac3[3] <<  "\t " << ppac3[4] <<std::endl;
	
	//ppac effciency
	if(ppac1[4]>0){
		ppac_count[0]++;
		if(ppac1[0]>0 && ppac1[1]>0){
			ppac_count[1]++;
		}		
		if(ppac1[2]>0 && ppac1[3]>0){
			ppac_count[2]++;
		}
	}

	if(ppac2[4]>0){
		ppac_count[3]++;
		if(ppac2[0]>0 && ppac2[1]>0){
			ppac_count[4]++;
		}		
		if(ppac2[2]>0 && ppac2[3]>0){
			ppac_count[5]++;
		}
	}

	if(ppac3[4]>0){
		ppac_count[6]++;
		if(ppac3[0]>0 && ppac3[1]>0){
			ppac_count[7]++;
		}		
		if(ppac3[2]>0 && ppac3[3]>0){
			ppac_count[8]++;
		}
	}
	        hist_ppac1.Fill(ppacxpos[0],ppacypos[0]);
                hist_ppac2.Fill(ppacxpos[1],ppacypos[1]);
                hist_ppac3.Fill(ppacxpos[2],ppacypos[2]);

	//3 ppac
	if(ppacxpos[0]>-100 && ppacypos[0]>-100 && ppacxpos[1]>-100 && ppacypos[1]>-100 && ppacxpos[2]>-100 && ppacypos[2]>-100){
		LinearRegression(ppaczxpos,ppacxpos,3,xfitpar);
		LinearRegression(ppaczypos,ppacypos,3,yfitpar);
		tarx = xfitpar[0];
		tary = yfitpar[0];

		//hist_ppac1.Fill(ppacxpos[0],ppacypos[0]);
		//hist_ppac2.Fill(ppacxpos[1],ppacypos[1]);
		//hist_ppac3.Fill(ppacxpos[2],ppacypos[2]);
		hist_tarpoint.Fill(tarx,tary);
		FillLine(hist_xztrace,xfitpar[0],xfitpar[1]);
		FillLine(hist_yztrace,yfitpar[0],yfitpar[1]);
	}

	//tofde
	// if(t1plastict>0 && t2plastict>0 && t2sie>0){
	// 	hist_tofde.Fill(t2sie,t2plastict-t1plastict);
	// }

	//DSSD4
	for(int i=0;i<frontnum && i<backnum;i++){
		hist_pos_dssd4.Fill(front_strip_dssd4[i],back_strip_dssd4[i]);
		hist_strip_energy_dssd4.Fill(front_strip_dssd4[i],front_energy_dssd4[i]);
		hist_strip_energy_dssd4.Fill(back_strip_dssd4[i]+32,back_energy_dssd4[i]);
	}
	
	//T0 CsI
	for(int i=0;i<num_count;i++){
		hist_pos_csi.Fill(front_num_csi[i],back_num_csi[i]);
		hist_strip_energy_csi.Fill(num_csi[i],energy_csi[i]);
	}
	// hist_time_difference.Fill(front_time - back_time);
	current_time = ppac1[4];
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

	//initialize counts
	for(int i=0;i<9;i++){
		ppac_count[0]=0;
	}

	// create canvas
	TCanvas* canvas = new TCanvas("canvas", "Online", 0, 0, 900, 900);
	TCanvas* canvas2 = new TCanvas("canvas2", "Online2", 0, 0, 600, 900);
	// create histograms
	TH2F hist_ppac1("ppac1", "ppac1", 50, -50, 50, 50, -50, 50);
	TH2F hist_ppac2("ppac2", "ppac2", 50, -50, 50, 50, -50, 50);
	TH2F hist_ppac3("ppac3", "ppac3", 50, -50, 50, 50, -50, 50);
	TH2F hist_tarpoint("tarpoint", "tarpoint", 50, -50, 50, 50, -50, 50);
	TH2F hist_tofde("tofde", "tofde", 100, 0, 65600, 100, -10000, 10000);
	TH1F hist_hframe("hframe", "ppac efficiency", 100, 0, 100);

	TH2F hist_pos_dssd4("hs4", "Pixel on DSSD4", 32, 0, 32, 32, 0, 32);
	TH2F hist_strip_energy_dssd4("hs4e", "Strip and Energy of DSSD4", 64, 0, 64, 1000, 0, 66000);
	TH2F hist_pos_csi("hcsip", "Position of CsI", 6, 0, 6, 6, 0, 6);
	TH2F hist_strip_energy_csi("hcsie", "Strip and Energy of CsI", 36, 0, 36, 1000, 0, 66000);

    line1 = new TLatex(20,85,"");
    line2 = new TLatex(20,50,"");
    line3 = new TLatex(20,15,"");

	TH2F hist_xztrace("xztrace", "xztrace", 200, -800, 200, 100, -50, 50);
	TH2F hist_yztrace("yztrace", "yztrace", 200, -800, 200, 100, -50, 50);
	// draw
	canvas->Divide(3, 3);
	canvas->cd(1);
	gPad->SetLogz();
	hist_ppac1.Draw("colz");
	canvas->cd(2);
	gPad->SetLogz();
	hist_ppac2.Draw("colz");
	canvas->cd(3);
	gPad->SetLogz();
	hist_ppac3.Draw("colz");
	canvas->cd(6);
	gPad->SetLogz();
	hist_tarpoint.Draw("colz");
	// canvas->cd(5);
	// hist_tofde.Draw("colz");
	canvas->cd(9);
	hist_hframe.SetStats(0);
	hist_hframe.SetMaximum(100);
    hist_hframe.SetMinimum(0);
    hist_hframe.SetStats(0);
	

	TAxis *xaxis = hist_hframe.GetXaxis();
    xaxis->SetLabelSize(0);
    xaxis->SetTitleSize(0);
    xaxis->SetTitle("");
    xaxis->SetTickLength(0);
    xaxis->SetNdivisions(0);
    xaxis->SetAxisColor(kWhite);

    TAxis *yaxis = hist_hframe.GetYaxis();
    yaxis->SetLabelSize(0);
    yaxis->SetTitleSize(0);
    yaxis->SetTitle("");
    yaxis->SetTickLength(0);
    yaxis->SetNdivisions(0);
    yaxis->SetAxisColor(kWhite);

	hist_hframe.SetLineColor(kWhite);
	hist_hframe.Draw("");
	gPad->SetFrameLineWidth(0);
    gPad->SetFrameBorderSize(0);
    gPad->SetFrameFillColor(0);
	line1->Draw("same");
	line2->Draw("same");
	line3->Draw("same");

	TLine lppac1x(ppaczxpos[0],-25,ppaczxpos[0],25);
	lppac1x.SetLineColor(kRed);
	lppac1x.SetLineWidth(3);
	TLine lppac2x(ppaczxpos[1],-25,ppaczxpos[1],25);
	lppac2x.SetLineColor(kRed);
	lppac2x.SetLineWidth(3);
	TLine lppac3x(ppaczxpos[2],-25,ppaczxpos[2],25);
	lppac3x.SetLineColor(kRed);
	lppac3x.SetLineWidth(3);
	TLine lppac1y(ppaczypos[0],-25,ppaczypos[0],25);
	lppac1y.SetLineColor(kRed);
	lppac1y.SetLineWidth(3);
	TLine lppac2y(ppaczypos[1],-25,ppaczypos[1],25);
	lppac2y.SetLineColor(kRed);
	lppac2y.SetLineWidth(3);
	TLine lppac3y(ppaczypos[2],-25,ppaczypos[2],25);
	lppac3y.SetLineColor(kRed);
	lppac3y.SetLineWidth(3);

	TLine ltar(0,-15,0,15);
	ltar.SetLineColor(kBlack);
	ltar.SetLineWidth(3);

	canvas->cd(5);
	gPad->SetPad(0.0,0.34,0.67,0.67);
	hist_xztrace.Draw("colz");
	lppac1x.Draw("same");
	lppac2x.Draw("same");
	lppac3x.Draw("same");
	ltar.Draw("same");

	canvas->cd(8);
	gPad->SetPad(0.0,0.0,0.67,0.33);
	hist_yztrace.Draw("colz");
	lppac1y.Draw("same");
	lppac2y.Draw("same");
	lppac3y.Draw("same");
	ltar.Draw("same");

	canvas2->Divide(2,3);
	canvas2->cd(1);
	hist_pos_dssd4.Draw("colz");
	canvas2->cd(2);
	hist_pos_csi.Draw("colz");
	canvas2->cd(4);
	gPad->SetPad(0.0,0.34,1.0,0.67);
	hist_strip_energy_dssd4.Draw("colz");
	canvas2->cd(6);
	gPad->SetPad(0.0,0.0,1.0,0.34);
	hist_strip_energy_csi.Draw("colz");

	// handle signal
	std::unique_ptr<SignalHandler> signal_handler = HandleSignal(canvas);
	std::unique_ptr<SignalHandler> signal_handler2 = HandleSignal(canvas2);
	// histograms
	std::vector<TH1*> histograms = {
		&hist_ppac1,
		&hist_ppac2,
        &hist_ppac3,
		&hist_tarpoint,
		&hist_hframe,
		&hist_xztrace,
		&hist_yztrace,
		&hist_pos_dssd4,
		&hist_strip_energy_dssd4,
		&hist_pos_csi,
		&hist_strip_energy_csi
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
	int packnum=0;
	while (receiver.Alive()) {
		for (
			std::vector<DecodeEvent> *event = receiver.ReceiveEvent(time_window);
			event;
			event = receiver.ReceiveEvent(time_window)
		) {
			// std::cout<<"recieve package" << packnum<<std::endl;
			FillOnlineGraph(
				*event, last_time,
				hist_ppac1, hist_ppac2,hist_ppac3,hist_tarpoint,hist_hframe,hist_xztrace,hist_yztrace,
				hist_pos_dssd4,hist_strip_energy_dssd4,hist_pos_csi,hist_strip_energy_csi,
				last_time
			);
			//std::cout<<"fill package" << packnum<<std::endl;
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
