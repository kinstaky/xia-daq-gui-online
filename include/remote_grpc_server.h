#ifndef __REMOTE_GRPC_SERVER_H__
#define __REMOTE_GRPC_SERVER_H__

#include <grpcpp/grpcpp.h>

#include "pixie16.grpc.pb.h"
#include "external/daq/MainFrame.hh"

class Pixie16Service final : public easydaq::pixie16::CallbackService {
public:
	explicit Pixie16Service(MainFrame *frame);

	grpc::ServerUnaryReactor* GetState(
		grpc::CallbackServerContext *context,
		const easydaq::Request *request,
		easydaq::Reply *reply
	);

	grpc::ServerUnaryReactor* RunControl(
		grpc::CallbackServerContext *context,
		const easydaq::Action *action,
		easydaq::Reply *reply
	);
private:
	MainFrame *frame_;
};


class RemoteGrpcServer {
public:
	RemoteGrpcServer(MainFrame *frame);

	~RemoteGrpcServer();

private:
	Pixie16Service *service_;
};


#endif // __REMOTE_GRPC_SERVER_H__
