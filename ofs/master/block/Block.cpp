#include "Block.h"
#include "BlockManager.h"
#include "time_helper.h"
#include <math.h>

namespace ofs {
	void Block::Write(const std::function<void(ChunkServer*)>& fn) {
		ChunkServer * mainServer = nullptr;
		ChunkServer * lowerServer = nullptr;

		int64_t now = olib::GetTimeStamp();
		for (auto& bIs : _chunkServer) {
			if (bIs.version != _version)
				continue;

			if (mainServer)
				break;

			if (bIs.leaseTick > now)
				mainServer = bIs.server;
			else if (lowerServer == nullptr || lowerServer->BusyThen(bIs.server))
				lowerServer = bIs.server;
		}

		if (!mainServer)
			mainServer = lowerServer;
		
		if (mainServer) {
			BlockManager::Instance().AddLease(this, mainServer);

			fn(mainServer);
		}
	}
}
