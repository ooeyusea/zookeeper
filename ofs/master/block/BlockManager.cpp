#include "BlockManager.h"
#include "Block.h"

namespace ofs {
	bool BlockManager::Start(const olib::IXmlObject& root) {
		_blockCount = root["data"][0]["block"][0].GetAttributeInt32("count");
		return true;
	}

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

	void BlockManager::Clean(Block * block) {
		if (block->IsUsed())
			return;

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (block->IsUsed())
			return;

		_blocks.erase(block->GetId());
		delete block;
	}
}
