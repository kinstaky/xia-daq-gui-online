#ifndef TEST_GRPC_CLIENT_COMMON_H
#define TEST_GRPC_CLIENT_COMMON_H

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "pixie16.grpc.pb.h"

constexpr char kGrpcAddress[] = "127.0.0.1:22330";
constexpr int kStateRunning = 2;
constexpr int kActionRunStart = 1;

inline std::unique_ptr<easydaq::pixie16::Stub> CreateStub() {
	return easydaq::pixie16::NewStub(
		grpc::CreateChannel(kGrpcAddress, grpc::InsecureChannelCredentials())
	);
}

inline bool GetState(easydaq::pixie16::Stub *stub, easydaq::Reply &reply) {
	easydaq::Request request;
	grpc::ClientContext context;
	grpc::Status status = stub->GetState(&context, request, &reply);
	if (!status.ok()) {
		std::cerr << "GetState failed: " << status.error_message() << "\n";
		return false;
	}
	return true;
}

inline bool ToggleRun(easydaq::pixie16::Stub *stub, easydaq::Reply &reply) {
	easydaq::Action action;
	action.set_type(kActionRunStart);
	grpc::ClientContext context;
	grpc::Status status = stub->RunControl(&context, action, &reply);
	if (!status.ok()) {
		std::cerr << "RunControl failed: " << status.error_message() << "\n";
		return false;
	}
	return true;
}

inline void PrintReply(const easydaq::Reply &reply) {
	std::cout << "status=" << reply.status() << " run=" << reply.run() << "\n";
}

#endif
