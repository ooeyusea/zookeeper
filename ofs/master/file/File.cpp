#include "File.h"
#include "FileSystem.h"
#include "Block.h"
#include "chunk/ChunkService.h"

namespace ofs {
	Block * File::GetAppendBlock() {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_blocks.empty() || (*_blocks.rbegin())->GetSize() < FileSystem::Instance().GetBlockSize()) {
			Block * block = new Block((int32_t)_blocks.size());
			_blocks.push_back(block);

			return block;
		}

		return *_blocks.rbegin();
	}
}
