#ifndef __BLOCK_MANAGER_H__
#define __BLOCK_MANAGER_H__
#include "hnet.h"
#include "singleton.h"
#include <unordered_map>
#include "Block.h"
#include "XmlReader.h"

namespace ofs {
	class BlockManager : public olib::Singleton<BlockManager> {
	public:
		BlockManager() : _mutex(true) {}
		~BlockManager() {}

		bool Start(const olib::IXmlObject& root);

		inline Block * Get(int64_t id) {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			auto itr = _blocks.find(id);
			if (itr != _blocks.end()) {
				itr->second->Acquire();
				return itr->second;
			}
			return nullptr;
		}

		void DeleteFileBlock(int64_t fileId, int64_t maxIndex);

		Block * GetOrCreate(int64_t id);

		void Clean(Block * block);

		inline int32_t GetBlockCount() const { return _blockCount; }
		inline int64_t GetWriteLease() const { return _writeLease; }
		inline int64_t GetRecoverLease() const { return _recoverLease; }
		inline int64_t GetRecoverAndWriteInterval() const { return _recoverAndWriteInterval; }

	private:
		void StartRecoverBlock(int64_t interval);

	private:
		hn_shared_mutex _mutex;
		std::unordered_map<int64_t, Block*> _blocks;

		int32_t _blockCount = 0;
		int64_t _writeLease = 0;
		int64_t _recoverLease = 0;
		int64_t _recoverAndWriteInterval = 0;

		hn_ticker * _recoverBlockTimer = nullptr;
	};
}

#endif //__BLOCK_MANAGER_H__
