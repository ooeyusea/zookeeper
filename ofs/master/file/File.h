#ifndef __FILE_H__
#define __FILE_H__
#include "hnet.h"
#include "Node.h"
#include "uuid.h"

namespace ofs {
	#define TICK_FROM(id) ((uint32_t)((id >> 32) & 0xFFFFFFFF))
	#define SUBID_FROM(id) ((uint16_t)(id & 0xFFFF))

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
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			if (index < (uint32_t)_blocks.size())
				return _blocks[index];
			return nullptr;
		}

		Block * GetAppendBlock();

		inline void SetBlock(uint32_t index, Block * block) {
			std::lock_guard<hn_shared_mutex> guard(_mutex);
			if ((uint32_t)_blocks.size() <= index)
				_blocks.resize(index + 1, nullptr);

			_blocks[index] = block;
		}

		inline static int64_t GenerateFileId() {
			static std::atomic<int64_t> s_id = 0;

			uint32_t now = 0;
			while (true) {
				int64_t id = s_id;
				uint32_t idTick = TICK_FROM(id);
				uint32_t subId = SUBID_FROM(id);


				if (now < idTick)
					now = olib::GetTimeStamp() / 10000;

			}
		}

	private:
		olib::uuid::UUID _uuid;
		std::vector<Block *> _blocks;
	};
}

#endif //__OFS_MASTER_H__
