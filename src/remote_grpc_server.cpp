#include "include/remote_grpc_server.h"

constexpr int kStateIdle = 3;
constexpr int kStateRunning = 2;
constexpr int kStateError = 1;
constexpr int kStateOff = 0;
constexpr int kActionRunNumber = 0;
constexpr int kActionRunStart = 1;
constexpr int kActionRunChange = 2;

RongPixie16Service::RongPixie16Service(MainFrame *frame)
: frame_(frame) {
}

grpc::ServerUnaryReactor* RongPixie16Service::GetState(
	grpc::CallbackServerContext *context,
    const rong::Request *request,
    rong::Reply *reply
) {
	reply->set_status(frame_->IsRunning() ? kStateRunning : kStateIdle);
	auto *reactor = context->DefaultReactor();
	reactor->Finish(grpc::Status::OK);
	return reactor;
}


grpc::ServerUnaryReactor* RongPixie16Service::RunControl(
	grpc::CallbackServerContext *context,
	const rong::Action *action,
	rong::Reply *reply
) {
	if (action->type() == kActionRunNumber) {
		// get run number
		reply->set_status(frame_->RunNumber());
	} else if (action->type() == kActionRunStart) {
		// start new run
		frame_->StartRun();
		reply->set_status(frame_->IsRunning() ? 1 : 0);
	} else if (action->type() == kActionRunChange) {
		if (frame_->IsRunning()) {
			reply->set_status(-1);
		} else {
			frame_->ChangeRunNumber(action->option());
			reply->set_status(frame_->RunNumber());
		}
	}
	auto* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
}


RemoteGrpcServer::RemoteGrpcServer(MainFrame *frame)
: service_(nullptr) {
	// builder
	std::string server_address("0.0.0.0:22331");
	grpc::ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

	// service
	service_ = new RongPixie16Service(frame);
	builder.RegisterService(service_);
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	server->Wait();
}


RemoteGrpcServer::~RemoteGrpcServer() {
	if (service_) delete service_;
}