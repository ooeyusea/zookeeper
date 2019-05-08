#include "File.h"
#include "FileSystem.h"
#include "block/Block.h"
#include "chunk/ChunkService.h"

namespace ofs {
	Block * File::GetAppendBlock() {
		if (_blocks.empty() || (*_blocks.rbegin())->GetSize() < FileSystem::Instance().GetBlockSize()) {
			Block * block = new Block;
			_blocks.push_back(block);

			return block;
		}

		return *_blocks.rbegin();
	}
}
