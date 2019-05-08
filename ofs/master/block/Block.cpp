#include "Block.h"
#include "BlockManager.h"
#include "time_helper.h"
#include <math.h>
#include "chunk/ChunkService.h"
#include "file/FileSystem.h"

#define MINUTE 60 * 1000 * 1000

namespace ofs {
	ChunkServer * Block::Write() {
		if (_chunkServer.empty())
			_chunkServer = ChunkService::Instance().Distribute(FileSystem::Instance().GetBlockCount());

		BlockInChunkServer * mainServer = nullptr;
		BlockInChunkServer * lowerServer = nullptr;

		int64_t now = olib::GetTimeStamp();
		for (auto& bIs : _chunkServer) {
			if (bIs.version != _version)
				continue;

			if (mainServer)
				break;

			if (bIs.leaseTick > now)
				mainServer = &bIs;
			else if (lowerServer == nullptr || lowerServer->server->BusyThen(bIs.server))
				lowerServer = &bIs;
		}

		if (!mainServer)
			mainServer = lowerServer;
		
		if (mainServer)
			mainServer->leaseTick = now + MINUTE;

		return mainServer->server;
	}
}
