#include "DataNodeService.h"
#include "block/Block.h"
#include "block/BlockManager.h"
#include "file/File.h"
#include "file/FileSystem.h"
#include "DataCluster.h"
#include "cluster/DefaultDataCluster.h"

#define MASTER_NODE 0
namespace ofs {
	bool DataNodeService::Start(const olib::IXmlObject& root) {
		std::string cluster = root["chunk_server"][0].GetAttributeString("cluster");
		const char * host = root["chunk_server"][0].GetAttributeString("host");
		int32_t port = root["chunk_server"][0].GetAttributeInt32("port");

		int32_t size = root["node"][0]["size"][0].GetAttributeInt32("val");

		_dataCluster = new DefaultDataCluster;

		_queue = new mq::MessageQueue(MASTER_NODE, size);
		if (!_queue->Listen(host, port))
			return false;

		_queue->Register<c2m::Register>(std::bind(&DataNodeService::OnRegister, *this, std::placeholders::_1));

		_queue->Register<c2m::ReportBlock>(std::bind(&DataNodeService::OnReport, *this, std::placeholders::_1));
		_queue->Register<c2m::UpdataBlock>(std::bind(&DataNodeService::OnUpdate, *this, std::placeholders::_1));
		_queue->Register<c2m::CleanComplete>(std::bind(&DataNodeService::OnClean, *this, std::placeholders::_1));

		_queue->Register<c2m::Heartbeat>(std::bind(&DataNodeService::OnHeartbeat, *this, std::placeholders::_1));
		return true;
	}

	std::vector<DataNode*> DataNodeService::Distribute(const std::vector<DataNode*>& old) {
		int32_t blockCount = BlockManager::Instance().GetBlockCount();
		if (old.size() >= blockCount)
			return {};

		hn_shared_lock<hn_shared_mutex> lock(_mutex);
		return _dataCluster->Distribute(old);
	}

	void DataNodeService::OnRegister(const c2m::Register& req) {
		std::unique_lock<hn_shared_mutex> lock(_mutex);
		DataNode * dataNode = _dataCluster->Register(req.id(), req.rack(), req.dc(), req.extend());
		assert(dataNode);

		dataNode->SetHost(req.outpost().host());
		dataNode->SetPort(req.outpost().port());

		//to update data node


		lock.unlock();
	}

	void DataNodeService::OnReport(const c2m::ReportBlock& req) {

	}

	void DataNodeService::OnUpdate(const c2m::UpdataBlock& req) {
		Block * block = BlockManager::Instance().Get(req.block().id());
		if (!block) {
			return;
		}

		File * file = FileSystem::Instance().GetFile(FILE_ID_FROM_BLOCK(block->GetId()));
		if (!file) {
			BlockManager::Instance().Clean(block);
			return;
		}

		block->UpdateReplica(req.id(), req.block().version(), req.block().size());
		block->Release();
	}

	void DataNodeService::OnClean(const c2m::CleanComplete& ntf) {

	}

	void DataNodeService::OnHeartbeat(const c2m::Heartbeat& ntf) {

	}
}
