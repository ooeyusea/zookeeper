#ifndef __STRING_ID_H__
#define __STRING_ID_H__

namespace olib {
	inline uint32_t BKDRHash(const char *str) {
		uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
		uint32_t hash = 0;

		while (*str)
			hash = hash * seed + (*str++);

		return (hash & 0x7FFFFFFF);
	}
}
#endif //__STRING_ID_H__
