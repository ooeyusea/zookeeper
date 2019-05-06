#ifndef __FILE_H__
#define __FILE_H__
#include "hnet.h"
#include "Node.h"
#include "uuid.h"

namespace ofs {
	class Block;
	class File : public Node {
	public:
		File() : Node(false) {
			_uuid = olib::uuid::UUID::Generate();
		}
		~File() {}

		virtual void DoNotWantToObject() {}

		template <typename AR>
		inline void Archive(AR& ar) {
			Node::Archive(ar);
			ar & _uuid;
		}

		inline const olib::uuid::UUID& GetUUID() const { return _uuid; }


		inline Block * GetBlock(uint32_t index) const { 
			if (index < (uint32_t)_blocks.size())
				return _blocks[index];
			return nullptr;
		}

		Block * GetAppendBlock();

		inline void SetBlock(uint32_t index, Block * block) {
			if ((uint32_t)_blocks.size() <= index)
				_blocks.resize(index + 1, nullptr);

			_blocks[index] = block;
		}

	private:
		olib::uuid::UUID _uuid;
		std::vector<Block *> _blocks;
	};
}

#endif //__OFS_MASTER_H__
