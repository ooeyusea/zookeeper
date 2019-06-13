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
		std::string cluster = root["cluster"][0].GetAttributeString("name");
		const char * host = root["cluster"][0].GetAttributeString("host");
		int32_t port = root["cluster"][0].GetAttributeInt32("port");

		int32_t size = root["node"][0]["size"][0].GetAttributeInt32("val");

		_dataCluster = new DefaultDataCluster;

		_queue = new mq::MessageQueue(MASTER_NODE, size);
		if (!_queue->Listen("0.0.0.0", port))
			return false;

		_queue->Register<c2m::Register>(std::bind(&DataNodeService::OnRegister, this, std::placeholders::_1));

		_queue->Register<c2m::ReportBlock>(std::bind(&DataNodeService::OnReport, this, std::placeholders::_1));
		_queue->Register<c2m::UpdataBlock>(std::bind(&DataNodeService::OnUpdate, this, std::placeholders::_1));
		_queue->Register<c2m::CleanComplete>(std::bind(&DataNodeService::OnClean, this, std::placeholders::_1));

		_queue->Register<c2m::Heartbeat>(std::bind(&DataNodeService::OnHeartbeat, this, std::placeholders::_1));
		return true;
	}

	std::vector<DataNode*> DataNodeService::Distribute(const std::vector<DataNode*>& old, const std::vector<DataNode*>& except) {
		int32_t blockCount = BlockManager::Instance().GetBlockCount();
		if (old.size() >= blockCount)
			return {};

		hn_shared_lock<hn_shared_mutex> lock(_mutex);
		return _dataCluster->Distribute(old, except);
	}

	std::vector<DataNode*> DataNodeService::SelectUnnecessary(std::vector<DataNode*>&& old) {
		int32_t blockCount = BlockManager::Instance().GetBlockCount();
		if (old.size() <= blockCount)
			return {};

		hn_shared_lock<hn_shared_mutex> lock(_mutex);
		return _dataCluster->SelectUnnecessary(old);
	}

	void DataNodeService::OnRegister(const c2m::Register& req) {
		std::unique_lock<hn_shared_mutex> lock(_mutex);
		DataNode * dataNode = _dataCluster->Register(req.id(), req.rack(), req.dc(), req.extend());
		assert(dataNode);

		dataNode->SetHost(req.outpost().host());
		dataNode->SetPort(req.outpost().port());

		//to update data node
		dataNode->SetCpu(req.node().cpu());
		dataNode->SetRss(req.node().rss());
		dataNode->SetVss(req.node().vss());
		dataNode->SetDisk(req.node().disk());
		dataNode->SetFault(req.node().fault());

		dataNode->UpdateTick();

		lock.unlock();
	}

	void DataNodeService::OnReport(const c2m::ReportBlock& req) {
		for (auto& blockInfo : req.blocks()) {
			File* file = FileSystem::Instance().GetFile(FILE_ID_FROM_BLOCK(blockInfo.id()));
			if (!file) {
				c2m::CleanBlock ntf;
				ntf.set_blockid(blockInfo.id());

				DataNodeService::Instance().GetSender()->Send(req.id(), &ntf);
				continue;
			}

			Block* block = BlockManager::Instance().GetOrCreate(blockInfo.id());
			if (!block) {
				file->Release();
				return;
			}

			DataNode* server = _dataCluster->Get(req.id());
			uint32_t newSize = block->ReportReplica(server, blockInfo.version(), blockInfo.size(), blockInfo.fault());
			block->Release();

			if (newSize > 0) {
				uint32_t fileSize = FileSystem::Instance().CalcFileSize(INDEX_FROM_BLOCK(block->GetId()), newSize);
				file->UpdateSize(fileSize);
			}

			file->Release();
		}
	}

	void DataNodeService::OnUpdate(const c2m::UpdataBlock& req) {
		Block * block = BlockManager::Instance().Get(req.block().id());
		if (!block) {
			return;
		}

		File * file = FileSystem::Instance().GetFile(FILE_ID_FROM_BLOCK(block->GetId()));
		if (!file) {
			block->Release();
			BlockManager::Instance().Clean(block);
			return;
		}

		uint32_t newSize = block->UpdateReplica(req.id(), req.block().version(), req.block().size(), req.block().fault());
		block->Release();

		if (newSize > 0) {
			uint32_t fileSize = FileSystem::Instance().CalcFileSize(INDEX_FROM_BLOCK(block->GetId()), newSize);
			file->UpdateSize(fileSize);
		}

		file->Release();
	}

	void DataNodeService::OnClean(const c2m::CleanComplete& ntf) {
		Block* block = BlockManager::Instance().Get(ntf.blockid());
		if (!block) {
			return;
		}

		block->ClearReplica(ntf.id());
		block->Release();
	}

	void DataNodeService::OnHeartbeat(const c2m::Heartbeat& ntf) {
		std::unique_lock<hn_shared_mutex> lock(_mutex);
		DataNode* dataNode = _dataCluster->Get(ntf.id());
		assert(dataNode);

		dataNode->SetCpu(ntf.node().cpu());
		dataNode->SetRss(ntf.node().rss());
		dataNode->SetVss(ntf.node().vss());
		dataNode->SetDisk(ntf.node().disk());
		dataNode->SetFault(ntf.node().fault());
		dataNode->UpdateTick();
	}
}
