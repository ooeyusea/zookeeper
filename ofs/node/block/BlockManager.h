#ifndef __OFS_NODE_H__
#define __OFS_NODE_H__
#include "hnet.h"
#include "singleton.h"
#include "XmlReader.h"
#include "lru_cache.h"
#include "Block.h"
#include "LocalFile.h"

namespace ofs {
	class BlockManager : public olib::Singleton<BlockManager> {
	public:
		BlockManager();
		virtual ~BlockManager() {}

		bool Start(const olib::IXmlObject& object);

		LocalFile * GetBlockFile(int32_t blockId);
		LocalFile * GetBlockMetaFile(int32_t blockId);

		inline Block * Get(int64_t blockId, bool create = false) {
			std::lock_guard<hn_mutex> guard(_mutexBlock);
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

	private:
		hn_mutex _mutex;
		olib::LRUCache<std::string, LocalFile> _cache;
		std::string _blockPath;

		hn_mutex _mutexBlock;
		std::unordered_map<int64_t, Block *> _blocks;
	};
}

#endif //__OFS_NODE_H__
