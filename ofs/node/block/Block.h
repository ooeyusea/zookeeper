#ifndef __BLOCK_H__
#define __BLOCK_H__
#include "hnet.h"
#include "RefObject.h"

namespace ofs {
	struct BlockInfo {
		int64_t id;
		int64_t version;
		int32_t size;
	};

	class Block : public RefObject {
	public:
		Block(int64_t id) : _mutex(true) { _info = { id, 0, 0 }; }
		~Block() {}

		inline int64_t GetId() const { return _info.id; }

		inline int64_t GetVersion() const { return _info.version; }
		inline void SetVersion(int64_t val) { _info.version = val; }

		inline int32_t GetSize() const { return _info.size; }
		inline void SetSize(int32_t val) { _info.size = val; }

		inline bool IsFault() const { return _fault; }
		inline void SetFault(bool val) { _fault = val; }

		int32_t Read(int32_t offset, int32_t size, std::string& data);
		int32_t Write(int64_t exceptVersion, int64_t newVersion, int32_t offset, const std::string& data, bool strict = false);
		int32_t Append(int64_t exceptVersion, int64_t newVersion, const std::string& data, bool strict = false);
		void Remove();

		void StartRecover(int64_t version, int64_t lease, int32_t copyTo);

	private:
		BlockInfo _info;
		bool _fault = false;

		hn_shared_mutex _mutex;
	};
}

#endif //__OFS_NODE_H__
