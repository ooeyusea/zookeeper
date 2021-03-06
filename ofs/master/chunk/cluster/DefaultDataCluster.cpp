#include "DefaultDataCluster.h"
#include "block/Block.h"
#include "block/BlockManager.h"
#include "file/File.h"
#include "file/FileSystem.h"
#include <algorithm>
#include <random>

namespace ofs {
	DataNode * Rack::ChooseChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add) {
		if (_chunkServers.empty())
			return nullptr;

		int32_t r = (int32_t)(rand() % _chunkServers.size());
		int32_t c = (int32_t)_chunkServers.size();
		while (c--) {
			r = (int32_t)((r + 1) % _chunkServers.size());

			auto * cs = _chunkServers[r];
			if (std::find(old.begin(), old.end(), cs) != old.end())
				continue;

			if (std::find(except.begin(), except.end(), cs) != except.end())
				continue;

			if (std::find(add.begin(), add.end(), cs) != add.end())
				continue;

			if (!cs->IsUseAble())
				continue;

			add.push_back(cs);
			return cs;
		}

		return nullptr;
	}

	DataNode * DataCenter::ChooseChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add, Rack * exclude) {
		if (_racks.empty())
			return nullptr;

		int32_t r = (int32_t)(rand() % _racks.size());
		int32_t c = (int32_t)_racks.size();
		while (c--) {
			if (_racks[r] == exclude)
				r = (int32_t)((r + 1) % _racks.size());

			DataNode * ret = _racks[r]->ChooseChunkServer(old, except, add);
			if (ret)
				return ret;

			r = (int32_t)((r + 1) % _racks.size());
		}

		return nullptr;
	}

	bool DefaultDataCluster::Start(const olib::IXmlObject& root) {
		return true;
	}

	DataNode* DefaultDataCluster::Get(int32_t id) {
		auto itr = _nodes.find(id);
		if (itr != _nodes.end())
			return itr->second;
		return nullptr;
	}

	DataNode * DefaultDataCluster::Register(int32_t id, int32_t rack, int32_t dc, const std::string& extend) {
		DataNode * node = Get(id);
		if (!node) {
			auto itr = std::lower_bound(_dataCenters.begin(), _dataCenters.end(), dc, [](DataCenter* dc, int32_t id) {
				return dc->GetId() < id;
			});

			DataCenter* dataCenter = nullptr;
			if (itr != _dataCenters.end() && (*itr)->GetId() == dc)
				dataCenter = *itr;
			else {
				dataCenter = new DataCenter(dc);
				_dataCenters.insert(itr, dataCenter);
			}

			node = dataCenter->Get(rack, true)->Get(id, true);

			_nodes[id] = node;
		}
		return node;
	}

	std::vector<DataNode*> DefaultDataCluster::Distribute(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except) {
		int32_t blockCount = BlockManager::Instance().GetBlockCount();
		if (old.size() >= blockCount)
			return {};

		std::vector<DataNode*> ret;
		DataNode * local = ChooseLocalChunkServer(old, except, ret);
		if (!local || blockCount == 1)
			return ret;

		DataNode * second = ChooseRemoteRackChunkServer(local, old, except, ret);
		if (second) {
			if (blockCount == 2)
				return ret;

			DataNode * third = ChooseLocalRackChunkServer(second, old, except, ret);
			if (third && blockCount == 3)
				return ret;
		}

		if (blockCount > 3) {
			for (int32_t i = 3; i < blockCount; ++i) {
				if (!ChooseRandomChunkServer(old, except, ret, nullptr))
					return ret;
			}
		}

		return ret;
	}

	DataNode * DefaultDataCluster::ChooseOne(DataNode* except) {
		std::vector<DataNode*> ret;
		if (ChooseRandomChunkServer({}, { except }, ret, nullptr))
			return ret.front();

		return nullptr;
	}

	std::vector<DataNode*> DefaultDataCluster::SelectUnnecessary(std::vector<DataNode*>&& old) {
		std::vector<RackStat> stats;
		stats.reserve(old.size());

		for (DataNode * node : old) {
			Rack * nodeRack = static_cast<ChunkServer*>(node)->GetRack();

			auto itr = std::find_if(stats.begin(), stats.end(), [nodeRack](const RackStat& a) {
				return a.rack == nodeRack;
			});

			if (itr == stats.end())
				stats.emplace_back(nodeRack, 0);
			else
				++itr->count;
		}

		std::vector<RackStat>::iterator stat = stats.end();
		int32_t rate = 1;
		for (auto itr = stats.begin(); itr != stats.end(); ++itr) {
			if (itr->count >= 2) {
				if (stat == stats.end() || rand() % rate == 0)
					stat = itr;
				++rate;
			}
		}

		std::shuffle(old.begin(), old.end(), std::default_random_engine());

		int32_t cut = (int32_t)old.size() - BlockManager::Instance().GetBlockCount();
		if (stat == stats.end()) {
			while (cut--)
				old.pop_back();
		}
		else {
			int32_t rackCut = stat->count - 2;
			cut -= rackCut;

			auto itr = old.rbegin();
			while (itr != old.rend()) {
				if (static_cast<ChunkServer*>(*itr)->GetRack() == stat->rack) {
					if (rackCut > 0) {
						itr = std::vector<DataNode*>::reverse_iterator(old.erase((++itr).base()));
						--rackCut;
						continue;
					}
				}
				else {
					if (cut > 0) {
						itr = std::vector<DataNode*>::reverse_iterator(old.erase((++itr).base()));
						--cut;
						continue;
					}
				}

				++itr;
			}
		}

		return old;
	}

	DataNode * DefaultDataCluster::ChooseLocalChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add) {
		if (old.empty())
			return ChooseRandomChunkServer(old, except, add, nullptr);

		return old.front();
	}

	DataNode * DefaultDataCluster::ChooseLocalRackChunkServer(DataNode * base, const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add) {
		if (old.size() >= 2) {
			for (int32_t i = 1; i < (int32_t)old.size(); ++i) {
				if (static_cast<ChunkServer*>(old[i])->GetRack() != static_cast<ChunkServer*>(base)->GetRack())
					return old[i];
			}

			if (old.size() + add.size() >= BlockManager::Instance().GetBlockCount())
				return nullptr;
		}

		return static_cast<ChunkServer*>(base)->GetRack()->ChooseChunkServer(old, except, add);
	}

	DataNode * DefaultDataCluster::ChooseRemoteRackChunkServer(DataNode * base, const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add) {
		DataNode * ret = static_cast<ChunkServer*>(base)->GetRack()->GetDataCenter()->ChooseChunkServer(old, except, add, static_cast<ChunkServer*>(base)->GetRack());
		if (ret)
			return ret;

		return ChooseRandomChunkServer(old, except, add, static_cast<ChunkServer*>(base)->GetRack()->GetDataCenter());
	}

	DataNode * DefaultDataCluster::ChooseRandomChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add, DataCenter * exclude) {
		if (_dataCenters.empty())
			return nullptr;

		int32_t r = (int32_t)(rand() % _dataCenters.size());
		int32_t c = (int32_t)_dataCenters.size();
		while (c--) {
			if (_dataCenters[r] == exclude)
				r = (int32_t)((r + 1) % (int32_t)_dataCenters.size());

			DataNode * ret = _dataCenters[r]->ChooseChunkServer(old, except, add, nullptr);
			if (ret)
				return ret;

			r = (int32_t)((r + 1) % (int32_t)_dataCenters.size());
		}

		return nullptr;
	}
}
