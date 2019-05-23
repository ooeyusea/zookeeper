#ifndef __OFS_BLOCK_MANAGER_H__
#define __OFS_BLOCK_MANAGER_H__
#include "hnet.h"
#include "singleton.h"
#include "XmlReader.h"
#include "lru_cache.h"
#include "Block.h"

namespace ofs {
	class BlockManager : public olib::Singleton<BlockManager> {
	public:
		BlockManager();
		virtual ~BlockManager() {}

		bool Start(const olib::IXmlObject& object);

		std::string GetBlockFile(int32_t blockId);
		std::string GetBlockMetaFile(int32_t blockId);

		inline Block * Get(int64_t blockId, bool create = false) {
			std::lock_guard<hn_mutex> guard(_mutex);
			auto itr = _blocks.find(blockId);
			if (itr != _blocks.end()) {
				itr->second->Acquire();
				return itr->second;
			}

			if (create) {
				Block * block = new Block(blockId);
				_blocks[blockId] = block;
				block->Acquire();
				return block;
			}
			return nullptr;
		}

		inline int32_t GetBatchSize() const { return _batchSize; }

	private:
		std::string _blockPath;
		int32_t _batchSize;

		hn_mutex _mutex;
		std::unordered_map<int64_t, Block *> _blocks;
	};
}

#endif //__OFS_BLOCK_MANAGER_H__
