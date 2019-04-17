#ifndef __UTIL_H__
#define __UTIL_H__
#include "hnet.h"
#include "coroutine_waiter.h"
#include "time_helper.h"
#include "socket_helper.h"

struct Server {
	int32_t id;
	std::string ip;
	int32_t electionPort;
	int32_t votePort;
};

#endif
