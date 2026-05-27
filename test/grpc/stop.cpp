#include "client_common.h"

int main() {
	auto stub = CreateStub();
	easydaq::Reply state_reply;
	if (!GetState(stub.get(), state_reply)) return 1;
	if (state_reply.status() != kStateRunning) {
		PrintReply(state_reply);
		return 0;
	}
	easydaq::Reply run_reply;
	if (!ToggleRun(stub.get(), run_reply)) return 1;
	PrintReply(run_reply);
	return 0;
}
