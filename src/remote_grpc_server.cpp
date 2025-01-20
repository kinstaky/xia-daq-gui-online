#include "include/remote_grpc_server.h"

RongPixie16Service::RongPixie16Service(MainFrame *frame)
: frame_(frame) {
}

grpc::ServerUnaryReactor* RongPixie16Service::GetState(
	grpc::CallbackServerContext *context,
    const rong::Request *request,
    rong::Reply *reply
) {
	reply->set_status(1);
	auto *reactor = context->DefaultReactor();
	reactor->Finish(grpc::Status::OK);
	return reactor;
}


grpc::ServerUnaryReactor* RongPixie16Service::RunControl(
	grpc::CallbackServerContext *context,
	const rong::Action *action,
	rong::Reply *reply
) {
	if (action->type() == kControlRunNumber) {
		// get run number
		reply->set_status(frame_->RunNumber());
	} else if (action->type() == kControlRunStatus) {
		// check run status
		reply->set_status(frame_->IsRunning() ? 1 : 0);
	} else if (action->type() == kControlRunStart) {
		// change run status
		frame_->StartRun();
		reply->set_status(frame_->IsRunning() ? 1 : 0);
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