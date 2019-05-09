#ifndef __REF_OBJECT_H__
#define __REF_OBJECT_H__
#include "hnet.h"

namespace ofs {
	class RefObject {
	public:
		RefObject() {}
		~RefObject() {}

		inline bool IsUsed() const { return _used.load(std::memory_order::memory_order_relaxed) > 0; }
		inline void Acquire() { _used.fetch_add(1, std::memory_order::memory_order_relaxed); }
		inline void Release() { _used.fetch_sub(1, std::memory_order::memory_order_relaxed); }

	private:
		std::atomic<int32_t> _used = 0;
	};
}

#endif //__REF_OBJECT_H__
