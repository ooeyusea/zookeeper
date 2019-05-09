#ifndef __BLOCK_H__
#define __BLOCK_H__
#include "hnet.h"
#include "RefObject.h"

namespace ofs {
	class ChunkServer;
	class Replica {
	public:
		Replica(ChunkServer * server) : _server(server) {}

		inline ChunkServer * GetServer() const { return _server; }

		inline void SetLease(int64_t val) { _leaseTick = val; }
		inline int64_t GetLease() const { return _leaseTick; }

		inline void SetVersion(int64_t val) { _version = val; }
		inline int64_t GetVersion() const { return _version; }

	private:
		ChunkServer * _server;
		int64_t _leaseTick = 0;
		int64_t _version = 0;
	};

	class Block : public RefObject {
	public:
		Block(int64_t id) : _id(id), _mutex(true), _size(0) {}
		~Block() {}

		inline int64_t GetId() const { return _id; }
		inline int32_t GetSize() const { return _size; }

		inline void Read(const std::function<void(ChunkServer*)>& fn) const {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			for (auto& bIs : _chunkServer) {
				if (bIs.GetVersion() == _version)
					fn(bIs.GetServer());
			}
		}

		ChunkServer * Write(std::vector<int32_t>& re, std::string& key);

	private:
		int64_t _id;
		mutable hn_shared_mutex _mutex;
		int64_t _version;
		int32_t _size;

		std::vector<Replica> _chunkServer;
	};
}

#endif //__BLOCK_H__
