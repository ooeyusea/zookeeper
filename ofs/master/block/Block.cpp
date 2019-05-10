#include "Block.h"
#include "time_helper.h"
#include <math.h>
#include "chunk/ChunkService.h"
#include "chunk/ChunkServer.h"
#include "file/FileSystem.h"

#define MINUTE 60 * 1000

namespace ofs {
	ChunkServer * Block::Write(std::vector<int32_t>& re) {
		if (_chunkServer.empty()) {
			std::lock_guard<hn_shared_mutex> guard(_mutex);
			auto servers = ChunkService::Instance().Distribute(FileSystem::Instance().GetBlockCount());
			for (auto * server : servers)
				_chunkServer.push_back({ server });
		}

		Replica * mainReplica = nullptr;
		Replica * lowerReplca = nullptr;

		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		int64_t now = olib::GetTimeStamp();
		for (auto& bIs : _chunkServer) {
			if (bIs.GetVersion() != _version)
				continue;

			if (mainReplica)
				break;

			if (bIs.GetLease() > now) {
				if (bIs.GetServer()->GetStatus() == ChunkServerStatus::CSS_BROKEN)
					return nullptr;
				mainReplica = &bIs;
			}
			else if (lowerReplca == nullptr && bIs.GetServer()->GetStatus() != ChunkServerStatus::CSS_BROKEN)
				lowerReplca = &bIs;
		}

		if (!mainReplica)
			mainReplica = lowerReplca;
		
		if (mainReplica) {
			int64_t tick = mainReplica->GetLease();
			if (tick <= now) {
				tick = now + MINUTE;
				mainReplica->SetLease(tick);
				_lease = tick;

				_expectVersion = NextVersion();
			}
			return mainReplica->GetServer();
		}

		return nullptr;
	}
}
