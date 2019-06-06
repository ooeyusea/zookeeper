#ifndef __FILE_H__
#define __FILE_H__
#include "hnet.h"
#include "Node.h"
#include "OfsId.h"
#include "RefObject.h"

namespace ofs {
	class File : public Node, public RefObject {
	public:
		File() : Node(false) {
			_id = IdGenerator::GenerateId();
		}
		File(int64_t id) : Node(false), _id(id) {}
		~File() {}

		virtual void DoNotWantToObject() {}

		template <typename AR>
		inline void Archive(AR& ar) {
			Node::Archive(ar);
			ar & _id;
		}

		inline int64_t GetId() const { return _id; }

		inline void UpdateSize(uint32_t size) {
			volatile uint32_t old = _size;
			while (old < size) {
#ifdef WIN32
				if (InterlockedCompareExchangeNoFence(&_size, size, old) == old) {
#else
				if (__sync_bool_compare_and_swap(&_size, old, size)) {
#endif
					break;
				}

				old = _size;
			}
		}

	private:
		int64_t _id;
	};
}

#endif //__OFS_MASTER_H__
