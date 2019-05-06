#ifndef __BLOCK_MANAGER_H__
#define __BLOCK_MANAGER_H__
#include "hnet.h"
#include "singleton.h"
#include <unordered_map>
#include "uuid.h"

namespace ofs {
	struct Block;
	class BlockManager : public olib::Singleton<BlockManager> {
	public:
		BlockManager() {}
		~BlockManager() {}

		void Add(Block * block);
		void Broken(Block * block);

	private:
		std::unordered_map<olib::uuid::UUID, Block*> _blocks;
	};
}

#endif //__BLOCK_MANAGER_H__
