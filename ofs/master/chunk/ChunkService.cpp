#include "ChunkService.h"
#include "ChunkServer.h"
#include "block/Block.h"
#include "block/BlockManager.h"
#include "file/File.h"
#include "file/FileSystem.h"

namespace ofs {
	bool ChunkService::Start(const olib::IXmlObject& root) {
		_rpc.AddService(this);

		const char * host = root["chunk_server"][0].GetAttributeString("host");
		int32_t port = root["chunk_server"][0].GetAttributeInt32("port");
		return _rpc.Start(host, port);
	}

	void ChunkService::RegisterChunkServer(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::RegisterChunkServerRequest* request,
		::ofs::c2m::RegisterChunkServerResponse* response,
		::google::protobuf::Closure* done) {

		ChunkServer * server = _servers[request->id() - 1];
		if (server)
			server->SetStatus(ChunkServerStatus::CSS_READY);
		else {
			ChunkServer * server = new ChunkServer;
#ifdef WIN32
			if (InterlockedCompareExchangePointerNoFence((void**)&_servers[request->id() - 1], server, 0) == nullptr) {
#else
			if (!__sync_bool_compare_and_swap((int64_t*)&_servers[request->id() - 1], 0, (int64_t)server)) {
#endif

				server->SetId(request->id());
				server->SetHost(request->host());
				server->SetPort(request->port());
			}
			else {
				delete server;
			}
		}
	}

	void ChunkService::Report(::google::protobuf::RpcController* controller,
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

		response->set_errcode(block->UpdateReplica(request->id(), request->blocks().version(), request->blocks().size(), response));
		block->Release();
	}

	void ChunkService::RenewLease(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::RenewLeaseRequest* request,
		::ofs::c2m::RenewLeaseResponse* response,
		::google::protobuf::Closure* done) {

		Block * block = BlockManager::Instance().Get(request->blockid());
		if (!block) {
			response->set_errcode(ofs::c2m::ErrorCode::EC_BLOCK_NOT_EXIST);
			return;
		}

		if (!block->UpdateLease([response](int64_t until, int64_t version, int64_t newVersion, const std::vector<int32_t>& replicas) {
			auto * lease = response->mutable_lease();
			lease->set_until(until);
			lease->set_version(version);
			lease->set_newversion(newVersion);

			for (auto r : replicas)
				lease->add_chunkservers(r);
		})) {
			response->set_errcode(ofs::c2m::ErrorCode::EC_REPLICA_DO_NOT_HAS_LEASE);
		}
		else
			response->set_errcode(ofs::c2m::ErrorCode::EC_OK);

		block->Release();
	}

	std::vector<ChunkServer*> ChunkService::Distribute(int32_t count) {
		std::vector<ChunkServer*> ret;
		return ret;
	}
}
