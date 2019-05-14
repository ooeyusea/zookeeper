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

	int32_t Block::UpdateReplica(int32_t chunkServerId, int64_t version, int32_t size, c2m::ReportResponse * response) {
		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		for (auto& bIs : _chunkServer) {
			if (bIs.GetServer()->GetId() == chunkServerId) {
				bIs.SetVersion(version);
				if (version > _version)
					_version = version;

				if (version > _expectVersion)
					_expectVersion = version;

				return c2m::ErrorCode::EC_OK;
			}
		}
		
		return c2m::ErrorCode::EC_BLOCK_CLEAN;
	}

	bool Block::UpdateLease(int32_t chunkServerId, c2m::RenewLeaseResponse * response) {
		int64_t now = olib::GetTimeStamp();

		std::lock_guard<hn_shared_mutex> guard(_mutex);
		for (auto& bIs : _chunkServer) {
			if (bIs.GetServer()->GetId() == chunkServerId) {
				if (bIs.GetLease() > now) {
					int64_t tick = bIs.GetLease() + MINUTE;
					bIs.SetLease(tick);
					_lease = tick;

					_expectVersion = NextVersion();

					auto * lease = response->mutable_lease();
					lease->set_until(tick);
					lease->set_version(_version);
					lease->set_newversion(_expectVersion);
				
					for (auto& bIs2 : _chunkServer) {
						if (bIs2.GetServer()->GetId() != chunkServerId && bIs2.GetServer()->IsUseAble())
							lease->add_chunkservers(bIs2.GetServer()->GetId());
					}

					return true;
				}

				return false;
			}
		}

		return false;
	}
}
