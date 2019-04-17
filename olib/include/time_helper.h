#ifndef __UTIL_H__
#define __UTIL_H__
#include "hnet.h"
#include <chrono>

namespace olib {
	inline int64_t GetTickCount() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	inline int64_t GetTimeStamp() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
}

#endif
