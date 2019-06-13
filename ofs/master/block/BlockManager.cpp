#include "BlockManager.h"
#include "Block.h"
#include "OfsId.h"

namespace ofs {
	bool BlockManager::Start(const olib::IXmlObject& root) {
		_blockCount = root["data"][0]["block"][0].GetAttributeInt32("count");
		_writeLease = root["data"][0]["lease"][0].GetAttributeInt32("write");
		_recoverLease = root["data"][0]["lease"][0].GetAttributeInt32("recover");
		_recoverAndWriteInterval = root["data"][0]["lease"][0].GetAttributeInt32("recover_and_write_interval");

		StartRecoverBlock();
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

	void BlockManager::DeleteFileBlock(int64_t fileId, int64_t maxIndex) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		for (int64_t i = 1; i <= maxIndex; ++i) {
			int64_t blockId = BLOCK_ID(fileId, i);

			auto itr = _blocks.find(blockId);
			if (itr != _blocks.end() && !itr->second->IsUsed()) {
				itr->second->BrocastCleanUp();

				delete itr->second;
				_blocks.erase(itr);
			}
		}
	}

	void BlockManager::Clean(Block * block) {
		if (block->IsUsed())
			return;

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (block->IsUsed())
			return;

		block->BrocastCleanUp();
		_blocks.erase(block->GetId());
		delete block;
	}

	void BlockManager::StartRecoverBlock() {
		//hn_fork [this]() {
		//	hn_wait IdleDetectService::Instance().GetNextIdleTime();
		//
		//	hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		//	for (auto itr = _blocks.begin(); itr != _blocks.end(); ++itr)
		//		itr->second->CheckReplica();
		//};
	}
}
