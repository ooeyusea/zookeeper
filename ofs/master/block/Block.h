#ifndef __BLOCK_H__
#define __BLOCK_H__
#include "hnet.h"
#include "RefObject.h"

namespace ofs {
	namespace c2m {
		class ReportResponse;
		class RenewLeaseResponse;
	}

	class DataNode;
	class Replica {
	public:
		Replica(DataNode * server) : _server(server) {}

		inline DataNode * GetServer() const { return _server; }

		inline void SetLease(int64_t val) { _leaseTick = val; }
		inline int64_t GetLease() const { return _leaseTick; }

		inline void SetVersion(int64_t val) { _version = val; }
		inline int64_t GetVersion() const { return _version; }

		inline void SetFault(bool fault) { _fault = fault; }
		inline bool IsVersion() const { return _fault; }

	private:
		DataNode * _server;
		int64_t _leaseTick = 0;
		int64_t _version = 0;
		bool _fault = false;
	};

	class Block : public RefObject {
	public:
		Block(int64_t id) : _id(id), _mutex(true), _size(0) {}
		~Block() {}

		inline int64_t GetId() const { return _id; }
		inline int32_t GetSize() const { return _size; }

		inline void Read(const std::function<void(DataNode*)>& fn) const {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			for (auto& bIs : _chunkServer) {
				if (bIs.GetVersion() == _version)
					fn(bIs.GetServer());
			}
		}

		DataNode * Write(std::vector<int32_t>& re);

		void BrocastCleanUp();

		inline int64_t NextVersion() {
			return ((((_version) >> 32) & 0xFFFFFFFF) + 1) << 32;
		}

		inline int64_t GetVersion() const { return _version; }
		inline int64_t GetExpectVersion() const { return _expectVersion; }
		inline int64_t GetLease() const { return _lease; }

		int32_t UpdateReplica(int32_t chunkServerId, int64_t version, int32_t size, bool fault);
		int32_t ReportReplica(DataNode* server, int64_t version, int32_t size, bool fault);
		void ClearReplica(int32_t chunkServerId);

	private:
		int64_t _id;
		mutable hn_shared_mutex _mutex;
		int64_t _version = 0;
		int64_t _expectVersion = 0;
		int64_t _lease = 0;
		int32_t _size;

		std::vector<Replica> _chunkServer;
	};
}

#endif //__BLOCK_H__
