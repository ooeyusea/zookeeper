#include "hnet.h"
#include "NodeManager.h"

void start(int32_t argc, char ** argv) {
	yarn::NodeManager manager;
	if (!manager.Start(argv[1])) {
		hn_error("node manager start failed");
		return;
	}

	hn_error("node manager start success");
}
