#include "Block.h"
#include "time_helper.h"
#include <math.h>
#include "chunk/ChunkService.h"
#include "chunk/ChunkServer.h"
#include "file/FileSystem.h"

#define MINUTE 60 * 1000

namespace ofs {
	ChunkServer * Block::Write() {
		if (_chunkServer.empty()) {
			std::lock_guard<hn_shared_mutex> guard(_mutex);
			auto servers = ChunkService::Instance().Distribute(FileSystem::Instance().GetBlockCount());
			for (auto * server : servers)
				_chunkServer.push_back({ server });
		}

		BlockInChunkServer * mainServer = nullptr;
		BlockInChunkServer * lowerServer = nullptr;

		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		int64_t now = olib::GetTimeStamp();
		for (auto& bIs : _chunkServer) {
			if (bIs.version != _version)
				continue;

			if (mainServer)
				break;

			if (bIs.leaseTick > now)
				mainServer = &bIs;
			else if (lowerServer == nullptr || lowerServer->server->BusyThen(*bIs.server))
				lowerServer = &bIs;
		}

		if (!mainServer)
			mainServer = lowerServer;
		
		if (mainServer)
			mainServer->leaseTick = now + MINUTE;

		mainServer->server->Accquire();
		return mainServer->server;
	}
}
