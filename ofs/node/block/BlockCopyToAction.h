#ifndef __BLOCK_COPYTO_ACTION_H__
#define __BLOCK_COPYTO_ACTION_H__
#include "hnet.h"

namespace ofs {
	class BlockCopyToAction {
	public:
		BlockCopyToAction() {}
		~BlockCopyToAction() {}

		void Start(int64_t id, int64_t version, int32_t size, int32_t copyTo);
	};
}

#endif //__BLOCK_COPYTO_ACTION_H__
