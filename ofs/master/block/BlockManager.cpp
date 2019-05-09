#include "BlockManager.h"
#include "Block.h"

namespace ofs {
	Block * BlockManager::GetOrCreate(int64_t id) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		auto itr = _blocks.find(id);
		if (itr != _blocks.end()) {
			itr->second->Acquire();
			return itr->second;
		}
		
		Block * block = new Block(id);
		_blocks[id] = block;
		block->Acquire();

		return block;
	}
}
