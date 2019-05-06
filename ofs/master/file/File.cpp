#include "File.h"
#include "FileSystem.h"
#include "block/Block.h"
#include "block/BlockManager.h"

namespace ofs {
	Block * File::GetAppendBlock() {
		if (_blocks.empty() || (*_blocks.rbegin())->GetSize() < FileSystem::Instance().GetBlockSize()) {
			Block * block = new Block;
			_blocks.push_back(block);

			BlockManager::Instance().Add(block);

			return block;
		}

		return *_blocks.rbegin();
	}
}
