#ifndef __DEFAULT_DATA_CLUSTER_H__
#define __DEFAULT_DATA_CLUSTER_H__
#include "hnet.h"
#include "../DataCluster.h"

namespace ofs {
	class Rack;
	class ChunkServer : public DataNode {
	public:
		ChunkServer(int32_t id, Rack * rack) : DataNode(id), _rack(rack) {}
		~ChunkServer() {}

		inline Rack * GetRack() const { return _rack; }

	private:
		Rack * _rack;
	};

	class DataCenter;
	class Rack {
	public:
		Rack(int32_t id, DataCenter * dc) : _id(id), _dc(dc) {}
		~Rack() {}

		int32_t GetId() const { return _id; }

		inline DataCenter * GetDataCenter() const { return _dc; }

		inline int32_t Size() const { return (int32_t)_chunkServers.size(); }
		inline ChunkServer * Get(int32_t id, bool create = false) {
			auto itr = std::lower_bound(_chunkServers.begin(), _chunkServers.end(), id, [](ChunkServer * rack, int32_t id) {
				return rack->GetId() < id;
			});

			if (itr != _chunkServers.end() && (*itr)->GetId() == id)
				return *itr;

			if (create) {
				ChunkServer * server = new ChunkServer(id, this);
				_chunkServers.insert(itr, server);
				return server;
			}

			return nullptr;
		}

		DataNode * ChooseChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add);

	private:
		int32_t _id;
		DataCenter * _dc;

		std::vector<ChunkServer*> _chunkServers;
	};

	class DataCenter {
	public:
		DataCenter(int32_t id) : _id(id) {}
		~DataCenter() {}

		int32_t GetId() const { return _id; }

		inline int32_t Size() const { return (int32_t)_racks.size(); }
		inline Rack * Get(int32_t id, bool create = false) {
			auto itr = std::lower_bound(_racks.begin(), _racks.end(), id, [](Rack * rack, int32_t id) {
				return rack->GetId() < id;
			});

			if (itr != _racks.end() && (*itr)->GetId() == id)
				return *itr;

			if (create) {
				Rack * rack = new Rack(id, this);
				_racks.insert(itr, rack);
				return rack;
			}

			return nullptr;
		}

		DataNode * ChooseChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add, Rack * exclude);

	private:
		int32_t _id;
		std::vector<Rack*> _racks;
	};

	struct RackStat {
		Rack * rack;
		int32_t count;

		RackStat(Rack * r, int32_t c ) : rack(r), count(c) {}
	};

	class DefaultDataCluster : public IDataCluster {
	public:
		DefaultDataCluster() {}
		~DefaultDataCluster() {}

		virtual bool Start(const olib::IXmlObject& root);

		virtual DataNode * Get(int32_t id);
		virtual DataNode * Register(int32_t id, int32_t rack, int32_t dc, const std::string& extend);
		virtual std::vector<DataNode*> Distribute(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except);
		virtual std::vector<DataNode*> SelectUnnecessary(std::vector<DataNode*>&& old);

	private:
		DataNode * ChooseLocalChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add);
		DataNode * ChooseLocalRackChunkServer(DataNode * base, const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add);
		DataNode * ChooseRemoteRackChunkServer(DataNode * base, const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add);
		DataNode * ChooseRandomChunkServer(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except, std::vector<DataNode*>& add, DataCenter * exclude);

	private:
		std::vector<DataCenter*> _dataCenters;
		std::unordered_map<int32_t, DataNode*> _nodes;
	};
}

#endif //__DEFAULT_DATA_CLUSTER_H__
