#include "Block.h"
#include "time_helper.h"
#include <math.h>
#include "chunk/DataNodeService.h"
#include "chunk/DataNode.h"
#include "file/FileSystem.h"

namespace ofs {
	DataNode * Block::Write(std::vector<int32_t>& re) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_chunkServer.empty()) {
			auto servers = DataNodeService::Instance().Distribute({});
			for (auto * server : servers)
				_chunkServer.push_back({ server });
		}

		Replica * mainReplica = nullptr;
		Replica * lowerReplca = nullptr;

		int64_t now = olib::GetTimeStamp();
		for (auto& bIs : _chunkServer) {
			if (bIs.GetVersion() != _version)
				continue;

			if (mainReplica)
				break;

			if (bIs.GetLease() > now) {
				if (!bIs.GetServer()->IsUseAble())
					return nullptr;
				mainReplica = &bIs;
			}
			else if (lowerReplca == nullptr && bIs.GetServer()->IsUseAble())
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

			for (auto& bIs : _chunkServer) {
				if (bIs.GetServer()->GetId() != mainReplica->GetServer()->GetId())
					re.emplace_back(bIs.GetServer()->GetId());
			}

			return mainReplica->GetServer();
		}

		return nullptr;
	}

	void Block::UpdateReplica(int32_t chunkServerId, int64_t version, int32_t size) {
		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		for (auto& bIs : _chunkServer) {
			if (bIs.GetServer()->GetId() == chunkServerId) {
				bIs.SetVersion(version);
				if (version > _version)
					_version = version;

				if (version > _expectVersion)
					_expectVersion = version;

				return;
			}
		}
	}
}
