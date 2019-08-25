#include "NodeService.h"
#include "api/OfsChunk.pb.h"
#include "proto/Chunk.pb.h"
#include "block/Block.h"
#include "block/BlockManager.h"
#include "client/ClientService.h"

namespace ofs {
	bool NodeService::Start(const olib::IXmlObject& root) {
		_id = root["node"][0]["id"][0].GetAttributeInt32("val");
		int32_t size = root["node"][0]["size"][0].GetAttributeInt32("val");
		int64_t heartBeat = root["node"][0]["hearbeat"][0].GetAttributeInt64("val");

		hn_info("node id : {}, service total size : {}", _id, size);
		
		std::string cluster = root["cluster"][0].GetAttributeString("name");
		_host = root["cluster"][0].GetAttributeString("host");
		_port = root["cluster"][0].GetAttributeInt32("port");

		hn_info("node harbor host {}:{}", _host, _port);

		std::string masterIp = root["cluster"][0]["master"][0].GetAttributeString("host");
		int32_t masterPort = root["cluster"][0]["master"][0].GetAttributeInt32("port");

		

		_rack = root["cluster"][0]["rack"][0].GetAttributeInt32("rack");
		_dc = root["cluster"][0]["rack"][0].GetAttributeInt32("dc");
		_extend = root["cluster"][0]["rack"][0].GetAttributeString("extend");

		hn_info("node rack {}:{}=>[{}]", _dc, _rack, _extend);

		_queue = new mq::MessageQueue(_id, size);
		if (!_queue->Listen("0.0.0.0", _port))
			return false;

		hn_info("node harbor listen port {}", _port);

		hn_info("node connect master {}:{}", masterIp, masterPort);
		_queue->Connect(MASTER_NODE, masterIp, masterPort, [this]() {
			hn_info("node connect master success");

			Register();
			BlockManager::Instance().Report();
		});

		_queue->Register<c2m::WriteNotify>(std::bind(&NodeService::OnWrite, this, std::placeholders::_1));
		_queue->Register<c2m::AppendNotify>(std::bind(&NodeService::OnAppend, this, std::placeholders::_1));

		_queue->Register<c2m::NeighborNotify>(std::bind(&NodeService::OnNeighborNotify, this, std::placeholders::_1));
		_queue->Register<c2m::NeighborGossip>(std::bind(&NodeService::OnNeighborGossip, this, std::placeholders::_1));

		_queue->Register<c2m::RecoverBlock>(std::bind(&NodeService::OnRecoverBlock, this, std::placeholders::_1));
		_queue->Register<c2m::CleanBlock>(std::bind(&NodeService::OnCleanBlock, this, std::placeholders::_1));

		_queue->Register<c2m::StartRecoverBlock>(std::bind(&NodeService::OnRecoverBlockStart, this, std::placeholders::_1));
		_queue->Register<c2m::RecoverBlockData>(std::bind(&NodeService::OnRecoverBlockData, this, std::placeholders::_1));
		_queue->Register<c2m::RecoverBlockComplete>(std::bind(&NodeService::OnRecoverBlockComplete, this, std::placeholders::_1));

		StartHeartbeat(heartBeat);
		return true;
	}

	void NodeService::Register() {
		c2m::Register req;
		req.set_id(_id);
		req.set_rack(_rack);
		req.set_dc(_dc);
		req.set_extend(_extend);

		auto* outpost = req.mutable_outpost();
		outpost->set_host(ClientService::Instance().GetHost());
		outpost->set_port(ClientService::Instance().GetPort());

		auto * harbor = req.mutable_harbor();
		harbor->set_host(_host);
		harbor->set_port(_port);

		auto * node = req.mutable_node();
		node->set_cpu(0);
		node->set_rss(0);
		node->set_vss(0);
		node->set_disk(0);
		node->set_fault(false);

		_queue->Send(MASTER_NODE, &req);
	}

	void NodeService::Write(const ::ofs::api::chunk::BlockLease& lease, int64_t oldVersion, int64_t newVersion, int32_t offset, const std::string& data) {
		c2m::WriteNotify ntf;
		ntf.set_blockid(lease.id());
		ntf.set_version(oldVersion);
		ntf.set_newversion(newVersion);
		ntf.set_offset(offset);
		ntf.set_data(data);

		for (auto neighbor : lease.chunkservers())
			_queue->Send(neighbor, &ntf);
	}

	void NodeService::Append(const ::ofs::api::chunk::BlockLease& lease, int64_t oldVersion, int64_t newVersion, const std::string& data) {
		c2m::AppendNotify ntf;
		ntf.set_blockid(lease.id());
		ntf.set_version(oldVersion);
		ntf.set_newversion(newVersion);
		ntf.set_data(data);

		for (auto neighbor : lease.chunkservers())
			_queue->Send(neighbor, &ntf);
	}

	void NodeService::Report(Block * block) {
		c2m::UpdataBlock req;
		req.set_id(_id);
		auto * status = req.mutable_block();
		status->set_id(block->GetId());
		status->set_version(block->GetVersion());
		status->set_size(block->GetSize());
		status->set_fault(block->IsFault());

		_queue->Send(MASTER_NODE, &req);
	}

	void NodeService::ReportClean(int64_t blockId) {
		c2m::CleanComplete req;
		req.set_id(_id);
		req.set_blockid(blockId);

		_queue->Send(MASTER_NODE, &req);
	}

	void NodeService::ReportBlocks(c2m::ReportBlock& report) {
		report.set_id(_id);

		_queue->Send(MASTER_NODE, &report);
	}

	void NodeService::OnWrite(const c2m::WriteNotify& ntf) {
		Block * block = BlockManager::Instance().Get(ntf.blockid());
		if (!block)
			return;

		if (block->Write(ntf.version(), ntf.newversion(), ntf.offset(), ntf.data(), true) == api::chunk::ErrorCode::EC_NONE)
			Report(block);

		block->Release();
	}

	void NodeService::OnAppend(const c2m::AppendNotify& ntf) {
		Block * block = BlockManager::Instance().Get(ntf.blockid(), true);
		if (!block)
			return;

		if (block->Append(ntf.version(), ntf.newversion(), ntf.data(), true) == api::chunk::ErrorCode::EC_NONE)
			Report(block);

		block->Release();
	}

	void NodeService::OnNeighborNotify(const c2m::NeighborNotify& ntf) {
		c2m::NeighborGossip gossip;
		auto * neighbor = gossip.mutable_neighbor();
		*neighbor = ntf.neighbor();

		_queue->Brocast(&gossip, MASTER_NODE);

		if (ntf.neighbor().id() > _id)
			_queue->Connect(ntf.neighbor().id(), ntf.neighbor().harbor().host(), ntf.neighbor().harbor().port());
	}

	void NodeService::OnNeighborGossip(const c2m::NeighborGossip& gossip) {
		if (gossip.neighbor().id() > _id)
			_queue->Connect(gossip.neighbor().id(), gossip.neighbor().harbor().host(), gossip.neighbor().harbor().port());
	}

	void NodeService::OnRecoverBlock(const c2m::RecoverBlock& cmd) {
		Block* block = BlockManager::Instance().Get(cmd.blockid());
		if (!block)
			return;

		for (auto node : cmd.copyto())
			block->StartRecover(cmd.version(), cmd.lease(), node);

		block->Release();
	}

	void NodeService::OnCleanBlock(const c2m::CleanBlock& cmd) {
		BlockManager::Instance().Clean(cmd.blockid());
	}

	void NodeService::OnRecoverBlockStart(const c2m::StartRecoverBlock& resize) {
		Block* block = BlockManager::Instance().Get(resize.blockid(), true);
		if (!block)
			return;

		block->Resize(resize.version(), resize.size());

		block->Release();
	}

	void NodeService::OnRecoverBlockData(const c2m::RecoverBlockData& data) {
		Block* block = BlockManager::Instance().Get(data.blockid());
		if (!block)
			return;

		block->RecoverData(data.version(), data.offset(), data.data());

		block->Release();
	}

	void NodeService::OnRecoverBlockComplete(const c2m::RecoverBlockComplete& complete) {
		Block* block = BlockManager::Instance().Get(complete.blockid());
		if (!block)
			return;

		if (block->CompleteRecover(complete.version())) {
			Report(block);
		}

		block->Release();
	}

	void NodeService::StartHeartbeat(int64_t heartBeat) {
		_heartBeatTimer = new hn_ticker(heartBeat);

		hn_info("node start heartbeat service for interval : {}", heartBeat);
		hn_fork[this]{
			for (auto t : *_heartBeatTimer) {
				c2m::Heartbeat req;
				req.set_id(_id);

				auto* node = req.mutable_node();
				node->set_cpu(0);
				node->set_rss(0);
				node->set_vss(0);
				node->set_disk(0);
				node->set_fault(false);

				_queue->Send(MASTER_NODE, &req);
			}
		};
	}
}
