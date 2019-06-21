#include "Block.h"
#include "time_helper.h"
#include <math.h>
#include "chunk/DataNodeService.h"
#include "chunk/DataNode.h"
#include "file/FileSystem.h"
#include "BlockManager.h"

namespace ofs {
	DataNode * Block::Write(std::vector<int32_t>& re) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		if (_chunkServer.empty()) {
			auto servers = DataNodeService::Instance().Distribute({}, {});
			for (auto * server : servers)
				_chunkServer.push_back({ server });
		}

		int64_t now = olib::GetTimeStamp();
		Replica * mainReplica = FindMainReplica(now);
		if (mainReplica) {
			int64_t tick = mainReplica->GetLease();
			if (tick <= now) {
				tick = now + BlockManager::Instance().GetWriteLease();
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

	Replica * Block::FindMainReplica(int64_t now) {
		Replica* mainReplica = nullptr;
		Replica* lowerReplca = nullptr;

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

		return mainReplica;
	}

	void Block::BrocastCleanUp() {
		c2m::CleanBlock ntf;
		ntf.set_blockid(_id);

		for (auto& bIs : _chunkServer)
			DataNodeService::Instance().GetSender()->Send(bIs.GetServer()->GetId(), &ntf);
	}

	void Block::CheckReplica(int32_t blockCount) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);
		int64_t now = olib::GetTimeStamp();
		if (_recoverLease > now)
			return;

		std::vector<DataNode*> has;
		std::vector<DataNode*> except;

		Replica* mainReplica = FindMainReplica(now);
		if (!mainReplica) {
			hn_warn("block {} is unuseful", _id);
			return;
		}

		if (mainReplica->GetLease() + BlockManager::Instance().GetRecoverAndWriteInterval() > now) {
			hn_trace("block {} is writed recently so wait for some time", _id);
			return;
		}

		c2m::CleanBlock ntf;
		ntf.set_blockid(_id);

		for (auto& bIs : _chunkServer) {
			if (!bIs.IsFault() && bIs.GetVersion() == _version && bIs.GetServer()->IsUseAble())
				has.push_back(bIs.GetServer());
			else {
				except.push_back(bIs.GetServer());

				DataNodeService::Instance().GetSender()->Send(bIs.GetServer()->GetId(), &ntf);
			}
		}

		if (has.size() < blockCount) {
			auto servers = DataNodeService::Instance().Distribute(has, except);
			if (!servers.empty()) {
				_recoverLease = now + BlockManager::Instance().GetRecoverLease();
				int32_t mainReplicaId = mainReplica->GetServer()->GetId();

				c2m::RecoverBlock cmd;
				cmd.set_blockid(_id);
				cmd.set_version(_version);
				cmd.set_lease(_recoverLease);

				for (auto* server : servers) {
					_chunkServer.push_back({ server });

					cmd.add_copyto(server->GetId());
				}

				DataNodeService::Instance().GetSender()->Send(mainReplicaId, &cmd);
			}
		}
		else if (has.size() > blockCount) {
			std::vector<DataNode*> ret = DataNodeService::Instance().SelectUnnecessary(std::move(has));
			for (auto& bIs : _chunkServer) {
				if (std::find(ret.begin(), ret.end(), bIs.GetServer()) != ret.end())
					DataNodeService::Instance().GetSender()->Send(bIs.GetServer()->GetId(), &ntf);
			}
		}
	}
}
