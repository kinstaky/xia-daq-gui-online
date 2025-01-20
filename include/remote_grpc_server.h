#ifndef __REMOTE_GRPC_SERVER_H__
#define __REMOTE_GRPC_SERVER_H__

#include <grpcpp/grpcpp.h>

#include "pixie16_rong.grpc.pb.h"
#include "external/daq/MainFrame.hh"

class RongPixie16Service final : public rong::pixie16::CallbackService {
public:
	explicit RongPixie16Service(MainFrame *frame);

	grpc::ServerUnaryReactor* GetState(
		grpc::CallbackServerContext *context,
		const rong::Request *request,
		rong::Reply *reply
	);

	grpc::ServerUnaryReactor* RunControl(
		grpc::CallbackServerContext *context,
		const rong::Action *action,
		rong::Reply *reply
	);
private:
	MainFrame *frame_;
};


class RemoteGrpcServer {
public:
	RemoteGrpcServer(MainFrame *frame);

	~RemoteGrpcServer();

private:
	RongPixie16Service *service_;
};


#endif // __REMOTE_GRPC_SERVER_H__