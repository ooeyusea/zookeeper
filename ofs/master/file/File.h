#ifndef __FILE_H__
#define __FILE_H__
#include "hnet.h"
#include "Node.h"
#include "OfsId.h"

namespace ofs {
	class File : public Node {
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

	private:
		int64_t _id;
	};
}

#endif //__OFS_MASTER_H__
