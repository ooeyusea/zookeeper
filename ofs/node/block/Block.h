#ifndef __BLOCK_H__
#define __BLOCK_H__
#include "hnet.h"
#include "RefObject.h"

namespace ofs {
	class Block : RefObject {
	public:
		Block(int64_t id) : _id(id) {}
		~Block() {}

		inline int64_t GetId() const { return _id; }

		inline int64_t GetVersion() const { return _version; }
		inline void SetVersion(int64_t val) { _version = val; }

		inline int32_t GetSize() const { return _size; }
		inline void SetSize(int32_t val) { _size = val; }

		int32_t Read(int32_t offset, int32_t size, std::string& data);
		int32_t Write(int32_t offset, const std::string& data);

	private:
		int64_t _id;
		int64_t _version;
		int32_t _size;
	};
}

#endif //__OFS_NODE_H__
