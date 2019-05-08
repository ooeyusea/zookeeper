#ifndef __BLOCK_H__
#define __BLOCK_H__
#include "hnet.h"
#include "uuid.h"

namespace ofs {
	class ChunkServer;
	class Block {
		struct BlockInChunkServer {
			olib::uuid::UUID version;
			int64_t leaseTick;
			ChunkServer * server;
		};
	public:
		Block() : _size(0) { _uuid = olib::uuid::UUID::Generate(); }
		~Block() {}

		inline olib::uuid::UUID GetUUID() const { return _uuid; }

		inline int32_t GetSize() const { return _size; }

		inline void Read(const std::function<void(ChunkServer*)>& fn) {
			for (auto& bIs : _chunkServer) {
				if (bIs.version == _version)
					fn(bIs.server);
			}
		}

		ChunkServer * Write();

	private:
		olib::uuid::UUID _uuid;
		olib::uuid::UUID _version;
		int32_t _size;

		std::vector<BlockInChunkServer> _chunkServer;
	};
}

#endif //__BLOCK_H__
