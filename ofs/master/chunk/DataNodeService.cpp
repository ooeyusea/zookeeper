#include "DataNodeService.h"
#include "block/Block.h"
#include "block/BlockManager.h"
#include "file/File.h"
#include "file/FileSystem.h"
#include "DataCluster.h"
#include "cluster/DefaultDataCluster.h"

namespace ofs {
	bool DataNodeService::Start(const olib::IXmlObject& root) {
		_rpc.AddService(this);

		const char * host = root["chunk_server"][0].GetAttributeString("host");
		int32_t port = root["chunk_server"][0].GetAttributeInt32("port");
		std::string cluster = root["chunk_server"][0].GetAttributeString("cluster");

		_dataCluster = new DefaultDataCluster;

		return _rpc.Start(host, port);
	}

	void DataNodeService::RegisterChunkServer(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::RegisterChunkServerRequest* request,
		::ofs::c2m::RegisterChunkServerResponse* response,
		::google::protobuf::Closure* done) {

		std::unique_lock<hn_shared_mutex> lock(_mutex);
		DataNode * dataNode = _dataCluster->Register(request->id(), request->rack(), request->dc(), request->extend());
		assert(dataNode);

		dataNode->SetHost(request->host());
		dataNode->SetPort(request->port());
		dataNode->SetRemain(request->remain());
		dataNode->SetFault(request->fault());
		dataNode->SetLoad(request->load());
		lock.unlock();

		response->set_ok(true);
	}

	void DataNodeService::Report(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::ReportRequest* request,
		::ofs::c2m::ReportResponse* response,
		::google::protobuf::Closure* done) {
		
		Block * block = BlockManager::Instance().Get(request->blocks().id());
		if (!block) {
			response->set_errcode(ofs::c2m::ErrorCode::EC_BLOCK_CLEAN);
			return;
		}

		File * file = FileSystem::Instance().GetFile(FILE_ID_FROM_BLOCK(block->GetId()));
		if (!file) {
			BlockManager::Instance().Clean(block);
			response->set_errcode(ofs::c2m::ErrorCode::EC_BLOCK_CLEAN);
			return;
		}

		response->set_errcode((c2m::ErrorCode)block->UpdateReplica(request->id(), request->blocks().version(), request->blocks().size(), response));
		block->Release();
	}

	void DataNodeService::RenewLease(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::RenewLeaseRequest* request,
		::ofs::c2m::RenewLeaseResponse* response,
		::google::protobuf::Closure* done) {

		Block * block = BlockManager::Instance().Get(request->blockid());
		if (!block) {
			response->set_errcode(ofs::c2m::ErrorCode::EC_BLOCK_NOT_EXIST);
			return;
		}

		if (!block->UpdateLease(request->id(), response))
			response->set_errcode(ofs::c2m::ErrorCode::EC_REPLICA_DO_NOT_HAS_LEASE);
		else
			response->set_errcode(ofs::c2m::ErrorCode::EC_OK);

		block->Release();
	}

	std::vector<DataNode*> DataNodeService::Distribute(const std::vector<DataNode*>& old) {
		int32_t blockCount = BlockManager::Instance().GetBlockCount();
		if (old.size() >= blockCount)
			return {};

		hn_shared_lock<hn_shared_mutex> lock(_mutex);
		return _dataCluster->Distribute(old);
	}
}
