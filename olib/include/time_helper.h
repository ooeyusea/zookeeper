#ifndef __TIME_HELPER_H__
#define __TIME_HELPER_H__
#include "hnet.h"
#include <chrono>
#include <iomanip>
#include <sstream>

#define SECOND 1000
#define MINUTE (60 * SECOND)
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)

namespace olib {
	inline int64_t GetTickCount() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	inline int64_t GetTimeStamp() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	inline int64_t DiffSecond() {
		int64_t now = GetTimeStamp();
		int64_t next = ((now / SECOND) + 1) * SECOND;
		return next - now;
	}

	inline std::string FomateTimeStamp(int64_t timestamp) {
		std::stringstream ss;
		std::time_t t(timestamp / SECOND);
		ss << std::put_time(std::localtime(&t), "%F %T");
		return ss.str();
	}
}

#endif //__TIME_HELPER_H__
