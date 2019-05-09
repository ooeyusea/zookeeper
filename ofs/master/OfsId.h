#ifndef __OFSID_H__
#define __OFSID_H__
#include "hnet.h"
#include "time_helper.h"

namespace olib {
	#define TICK_FROM_ID(id) ((id) >> 31)
	#define TICK_FROM_TIMESTAMP(ts) (((ts) / SECOND) & 0x1FFFFFFFF)
	#define SEQ_FROM_ID(id) ((id) & 0x3FFF)
	#define MAX_SEQ 0x3FFF
	#define BUILD_FILE_ID(t, subId) ((((t) & 0x1FFFFFFFF) << 31) | ((subId) & 0x3FFF))

	#define BLOCK_ID(fileId, index) ((fileId) | (((int64_t)(index)) << 10))

	struct IdGenerator {
		inline static int64_t GenerateId() {
			static std::atomic<int64_t> s_id = 0;
			while (true) {
				int64_t id = s_id.load(std::memory_order::memory_order_relaxed);
				int64_t idTick = TICK_FROM_ID(id);
				int64_t seq = SEQ_FROM_ID(id) + 1;
				int64_t nowTs = olib::GetTimeStamp();

				if (idTick < TICK_FROM_TIMESTAMP(nowTs)) {
					idTick = TICK_FROM_TIMESTAMP(nowTs);
					seq = 0;
				}
				else {
					if (seq >= MAX_SEQ) {
						hn_sleep(olib::DiffSecond());
						continue;
					}
				}

				int64_t newId = BUILD_FILE_ID(idTick, seq);
				if (s_id.compare_exchange_weak(id, newId, std::memory_order::memory_order_relaxed))
					return newId;
			}
		}
	};
}

#endif //__OFSID_H__
