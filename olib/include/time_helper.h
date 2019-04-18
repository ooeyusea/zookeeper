#ifndef __TIME_HELPER_H__
#define __TIME_HELPER_H__
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

#endif //__TIME_HELPER_H__
