#include "hnet.h"
#include "OfsMaster.h"
#include "args.h"

void start(int32_t argc, char ** argv) {



	ofs::Master master;
	if (!master.Start()) {
		hn_error("master start failed");
		return;
	}

	hn_info("master start success");
}