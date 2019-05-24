#include "NodeService.h"
#include "api/OfsChunk.pb.h"
#include "proto/Chunk.pb.h"
#include "block/Block.h"
#include "block/BlockManager.h"

namespace ofs {
	bool NodeService::Start(const olib::IXmlObject& root) {
		_id = root["node"][0]["id"][0].GetAttributeInt32("val");
		int32_t size = root["node"][0]["size"][0].GetAttributeInt32("val");
		std::string ip = root["node"][0]["host"][0].GetAttributeString("ip");
		int32_t port = root["node"][0]["host"][0].GetAttributeInt32("port");

		std::string masterIp = root["node"][0]["matser"][0].GetAttributeString("ip");
		int32_t masterPort = root["node"][0]["matser"][0].GetAttributeInt32("port");

		int64_t heartBeat = root["node"][0]["hear_beat"][0].GetAttributeInt64("val");

		_queue = new mq::MessageQueue(_id, size);
		if (!_queue->Listen(ip, port))
			return false;

		_queue->Connect(MASTER_NODE, masterIp, masterPort, [this]() {
			Register();
			BlockManager::Instance().Report();
		});

		_queue->Register<c2m::WriteNotify>(std::bind(&NodeService::OnWrite, *this, std::placeholders::_1));
		_queue->Register<c2m::AppendNotify>(std::bind(&NodeService::OnAppend, *this, std::placeholders::_1));

		_queue->Register<c2m::NeighborNotify>(std::bind(&NodeService::OnNeighborNotify, *this, std::placeholders::_1));
		_queue->Register<c2m::NeighborGossip>(std::bind(&NodeService::OnNeighborGossip, *this, std::placeholders::_1));

		_queue->Register<c2m::RecoverBlock>(std::bind(&NodeService::OnRecoverBlock, *this, std::placeholders::_1));
		_queue->Register<c2m::CleanBlock>(std::bind(&NodeService::OnCleanBlock, *this, std::placeholders::_1));

		StartHeartbeat(heartBeat);
		return true;
	}

	void NodeService::Register() {
		c2m::Register req;
		req.set_id(_id);

		//fullfill

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
		Block * block = BlockManager::Instance().Get(ntf.blockid());
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

	}

	void NodeService::OnCleanBlock(const c2m::CleanBlock& cmd) {
		BlockManager::Instance().Clean(cmd.blockid());
	}

	void NodeService::StartHeartbeat(int64_t heartBeat) {
		_heartBeatTimer = new hn_ticker(heartBeat);

		hn_fork[this]{
			for (auto t : *_heartBeatTimer) {
				c2m::Heartbeat req;
				req.set_id(_id);

				//fullfill

				_queue->Send(MASTER_NODE, &req);
			}
		};
	}
}
