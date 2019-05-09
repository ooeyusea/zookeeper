#ifndef __BLOCK_MANAGER_H__
#define __BLOCK_MANAGER_H__
#include "hnet.h"
#include "singleton.h"
#include <unordered_map>
#include "Block.h"

namespace ofs {
	class BlockManager : public olib::Singleton<BlockManager> {
	public:
		BlockManager() : _mutex(true) {}
		~BlockManager() {}

		inline Block * Get(int64_t id) {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			auto itr = _blocks.find(id);
			if (itr != _blocks.end()) {
				itr->second->Acquire();
				return itr->second;
			}
			return nullptr;
		}

		Block * GetOrCreate(int64_t id);

	private:
		hn_shared_mutex _mutex;
		std::unordered_map<int64_t, Block*> _blocks;
	};
}

#endif //__BLOCK_MANAGER_H__
