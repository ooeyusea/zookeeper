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

		_queue = new mq::MessageQueue(size);
		if (!_queue->Listen(ip, port))
			return false;

		_queue->Connect(masterIp, masterPort);

		_queue->Register<c2m::WriteNotify>(std::bind(&NodeService::OnWrite, *this, std::placeholders::_1));
		_queue->Register<c2m::AppendNotify>(std::bind(&NodeService::OnAppend, *this, std::placeholders::_1));
		return true;
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
		c2m::ReportRequest req;
		req.set_id(_id);
		auto * status = req.mutable_block();
		status->set_id(block->GetId());
		status->set_version(block->GetVersion());
		status->set_size(block->GetSize());

		_queue->Send(MASTER_NODE, &req);
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
}
