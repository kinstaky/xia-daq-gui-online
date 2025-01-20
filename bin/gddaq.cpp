#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <iceoryx_posh/runtime/posh_runtime.hpp>

#include "external/daq/MainFrame.hh"
#ifdef REMOTE_CONTROL_GRPC
#include "include/remote_grpc_server.h"
#endif

using namespace std;
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc, char **argv) {
	iox::runtime::PoshRuntime::initRuntime("gddaq-gui-online");

	TApplication theApp("App", &argc, argv);

	// check compiler
	if(sizeof(char) != 1) {
		std::cout << "sizeof(char) != 1 The current compiler is not "
			<< "suitable for running the program！\n";
	}
	if(sizeof(short) != 2) {
		std::cout << "sizeof(short) != 2 The current compiler is not "
			<< "suitable for running the program！\n";
	}
	if(sizeof(int) != 4) {
		std::cout << "sizeof(int) != 4 The current compiler is not "
			<< "suitable for running the program！\n";
	}

	MainFrame main_frame(gClient->GetRoot());
#ifdef REMOTE_CONTROL_GRPC
	std::thread grpc_thread(
		[](MainFrame *frame) {
			RemoteGrpcServer server(frame);
		},
		&main_frame
	);
#endif

	theApp.Run();

#ifdef REMOTE_CONTROL_GRPC
	grpc_thread.join();
#endif

	return 0;
}
