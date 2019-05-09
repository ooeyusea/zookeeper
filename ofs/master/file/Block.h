#ifndef __BLOCK_H__
#define __BLOCK_H__
#include "hnet.h"
#include "uuid.h"

namespace ofs {
	class ChunkServer;
	class Block {
		struct BlockInChunkServer {
			ChunkServer * server;
			olib::uuid::UUID version;
			int64_t leaseTick;
		};
	public:
		Block(int32_t index) : _index(index), _size(0) {}
		~Block() {}

		inline int32_t GetIndex() const { return _index; }
		inline int32_t GetSize() const { return _size; }

		inline void Read(const std::function<void(ChunkServer*)>& fn) const {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			for (auto& bIs : _chunkServer) {
				if (bIs.version == _version)
					fn(bIs.server);
			}
		}

		ChunkServer * Write();

	private:
		mutable hn_shared_mutex _mutex;
		int32_t _index;
		olib::uuid::UUID _version;
		int32_t _size;

		std::vector<BlockInChunkServer> _chunkServer;
	};
}

#endif //__BLOCK_H__
