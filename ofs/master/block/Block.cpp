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

	int32_t Block::UpdateReplica(int32_t chunkServerId, int64_t version, int32_t size, bool fault) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		auto itr = std::find_if(_chunkServer.begin(), _chunkServer.end(), [chunkServerId](auto& bIs) {
			return bIs.GetServer()->GetId() == chunkServerId;
		});
		
		if (itr != _chunkServer.end()) {
			itr->SetVersion(version);
			itr->SetFault(fault);

			if (!fault) {
				if (version > _version)
					_version = version;

				if (version > _expectVersion)
					_expectVersion = version;

				if (_size < size) {
					_size = size;
					return size;
				}
			}
		}
		return 0;
	}

	int32_t Block::ReportReplica(DataNode* server, int64_t version, int32_t size, bool fault) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		auto itr = std::find_if(_chunkServer.begin(), _chunkServer.end(), [server](auto& bIs) {
			return bIs.GetServer()->GetId() == server->GetId();
		});

		if (itr == _chunkServer.end())
			itr = _chunkServer.insert(_chunkServer.end(), { server });

		if (itr != _chunkServer.end()) {
			itr->SetVersion(version);
			itr->SetFault(fault);

			if (!fault) {
				if (version > _version)
					_version = version;

				if (version > _expectVersion)
					_expectVersion = version;

				if (_size < size) {
					_size = size;
					return size;
				}
			}
		}
		return 0;
	}

	void Block::ClearReplica(int32_t chunkServerId) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		std::remove_if(_chunkServer.begin(), _chunkServer.end(), [chunkServerId](auto& bIs) {
			return bIs.GetServer()->GetId() == chunkServerId;
		});
	}

	void Block::BrocastCleanUp() {
		c2m::CleanBlock ntf;
		ntf.set_blockid(_id);

		for (auto& bIs : _chunkServer)
			DataNodeService::Instance().GetSender()->Send(bIs.GetServer()->GetId(), &ntf);
	}
}
