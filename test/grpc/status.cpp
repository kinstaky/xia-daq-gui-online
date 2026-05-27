#include "client_common.h"

int main() {
	auto stub = CreateStub();
	easydaq::Reply reply;
	if (!GetState(stub.get(), reply)) return 1;
	PrintReply(reply);
	return 0;
}
