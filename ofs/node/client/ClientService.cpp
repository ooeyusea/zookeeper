#include "ClientService.h"
#include "block/BlockManager.h"
#include "node/NodeService.h"
#include "time_helper.h"

namespace ofs {
	bool ClientService::Start(const olib::IXmlObject& root) {
		_rpc.AddService(this);

		_host = root["client"][0].GetAttributeString("host");
		_port = root["client"][0].GetAttributeInt32("port");

		hn_info("listen for client {}:{}", _host, _port);
		return _rpc.Start("0.0.0.0", _port);
	}

	void ClientService::Read(::google::protobuf::RpcController* controller,
		const ::ofs::api::chunk::ReadRequest* request,
		::ofs::api::chunk::ReadResponse* response,
		::google::protobuf::Closure* done) {

		Block * block = BlockManager::Instance().Get(request->blockid());
		if (!block) {
			response->set_errcode(api::chunk::ErrorCode::EC_BLOCK_NOT_EIXST);
			return;
		}

		std::string * data = new std::string;
		int32_t ret = block->Read(request->offset(), BlockManager::Instance().GetBatchSize(), *data);

		response->set_errcode((api::chunk::ErrorCode)ret);
		response->set_allocated_data(data);

		block->Release();
	}

	void ClientService::Write(::google::protobuf::RpcController* controller,
		const ::ofs::api::chunk::WriteRequest* request,
		::ofs::api::chunk::WriteResponse* response,
		::google::protobuf::Closure* done) {

		//check key
		auto& lease = request->lease();
		if (lease.until() < olib::GetTimeStamp()) {
			response->set_errcode(api::chunk::ErrorCode::EC_LEASE_EXPIRE);
			return;
		}

		if (request->data().size() != BlockManager::Instance().GetBatchSize()) {
			response->set_errcode(api::chunk::ErrorCode::EC_WRITE_BLOCK_CHECK_SIZE_FAILED);
			return;
		}

		Block * block = BlockManager::Instance().Get(lease.id());
		if (!block) {
			response->set_errcode(api::chunk::ErrorCode::EC_BLOCK_NOT_EIXST);
			return;
		}

		int64_t oldVersion = block->GetVersion();
		int32_t ret = block->Write(lease.version(), lease.newversion(), request->offset(), request->data());
		response->set_errcode((api::chunk::ErrorCode)ret);

		if (ret == api::chunk::ErrorCode::EC_NONE) {
			NodeService::Instance().Write(lease, oldVersion, block->GetVersion(), request->offset(), request->data());
			NodeService::Instance().Report(block);
		}

		block->Release();
	}

	void ClientService::Append(::google::protobuf::RpcController* controller,
		const ::ofs::api::chunk::AppendRequest* request,
		::ofs::api::chunk::AppendResponse* response,
		::google::protobuf::Closure* done) {

		//check key
		auto& lease = request->lease();
		if (lease.until() < olib::GetTimeStamp()) {
			response->set_errcode(api::chunk::ErrorCode::EC_LEASE_EXPIRE);
			return;
		}

		if (request->data().size() != BlockManager::Instance().GetBatchSize()) {
			response->set_errcode(api::chunk::ErrorCode::EC_WRITE_BLOCK_CHECK_SIZE_FAILED);
			return;
		}

		Block * block = BlockManager::Instance().Get(lease.id(), true);
		if (!block) {
			response->set_errcode(api::chunk::ErrorCode::EC_BLOCK_NOT_EIXST);
			return;
		}

		int64_t oldVersion = block->GetVersion();
		int32_t ret = block->Append(lease.version(), lease.newversion(), request->data());
		response->set_errcode((api::chunk::ErrorCode)ret);

		if (ret == api::chunk::ErrorCode::EC_NONE) {
			NodeService::Instance().Append(lease, oldVersion, block->GetVersion(), request->data());
			NodeService::Instance().Report(block);
		}

		block->Release();
	}
}
