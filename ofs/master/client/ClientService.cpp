#include "ClientService.h"
#include "file/FileSystem.h"
#include "user/UserManager.h"
#include "chunk/DataNode.h"
#include "block/Block.h"
#include "block/BlockManager.h"
#include "OfsId.h"

namespace ofs {
	bool ClientService::Start(const olib::IXmlObject& root) {
		_rpc.AddService(this);

		const char * host = root["client"][0].GetAttributeString("host");
		int32_t port = root["client"][0].GetAttributeInt32("port");
		return _rpc.Start(host, port);
	}

	void ClientService::Login(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::LoginReq* request,
		::ofs::api::master::LoginResponse* response,
		::google::protobuf::Closure* done) {

		std::string token = UserManager::Instance().Login(request->name(), request->password());
		if (!token.empty()) {
			response->set_errcode(api::master::ErrorCode::EC_NONE);
			response->set_token(token);
		}
		else
			response->set_errcode(api::master::ErrorCode::EC_USER_OR_PASSWORD_ERROR);
	}

	void ClientService::MakeDir(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::MakeDirRequest* request,
		::ofs::api::master::MakeDirResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->directory().c_str(), request->name().c_str(), request->authority(), true);
			response->set_errcode((api::master::ErrorCode)ret);
			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Create(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::CreateFileRequest* request,
		::ofs::api::master::CreateFileResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->directory().c_str(), request->name().c_str(), request->authority(), false);
			response->set_errcode((api::master::ErrorCode)ret);
			UserManager::Instance().Release(user);
		}
	}

	void ClientService::List(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::ListRequest* request,
		::ofs::api::master::ListResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			std::vector<Node*> ret = FileSystem::Instance().Root().List(user, request->path().c_str());

			response->set_errcode(api::master::ErrorCode::EC_NONE);
			for (auto * node : ret) {
				auto * n = response->add_files();

				n->set_name(node->GetName());
				n->set_owner(node->GetOwner());
				n->set_group(node->GetOwnerGroup());
				n->set_authority(node->GetAuthority());
				n->set_size(node->GetSize());
				n->set_createtime(node->GetCreateTime());
				n->set_updatetime(node->GetUpdateTime());
				n->set_dir(node->IsDir());
			}

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Remove(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::RemoveRequest* request,
		::ofs::api::master::RemoveResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			int32_t ret = FileSystem::Instance().Root().Remove(user, request->path().c_str());
			response->set_errcode((api::master::ErrorCode)ret);
			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Status(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::FileStatusRequest* request,
		::ofs::api::master::FileStatusRespone* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			int32_t ret = FileSystem::Instance().Root().QueryNode(user, request->path().c_str(), [response](User * user, Node * node) {
				auto * n = response->mutable_file();

				n->set_name(node->GetName());
				n->set_owner(node->GetOwner());
				n->set_group(node->GetOwnerGroup());
				n->set_authority(node->GetAuthority());
				n->set_size(node->GetSize());
				n->set_createtime(node->GetCreateTime());
				n->set_updatetime(node->GetUpdateTime());
				n->set_dir(node->IsDir());

				return api::master::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::master::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Read(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::ReadRequest* request,
		::ofs::api::master::ReadResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			int32_t ret = FileSystem::Instance().Root().QueryNode(user, request->path().c_str(), [request, response](User * user, Node * node) {
				if (node->IsDir())
					return api::master::ErrorCode::EC_IS_DIR;

				File * file = static_cast<File*>(node);
				int32_t blockCount = FileSystem::Instance().CalcBlockCount(file->GetSize());
				if (request->blockindex() < 0 || request->blockindex() >= blockCount)
					return api::master::ErrorCode::EC_OUT_OF_RANGE;

				Block * block = BlockManager::Instance().Get(BLOCK_ID(file->GetId(), request->blockindex()));
				if (!block)
					return api::master::ErrorCode::EC_BLOCK_MISSING;

				response->set_id(block->GetId());

				block->Read([response](DataNode * server){
					auto * ep = response->add_eps();
					ep->set_host(server->GetHost());
					ep->set_port(server->GetPort());
				});

				block->Release();
				return api::master::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::master::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Write(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::WriteRequest* request,
		::ofs::api::master::WriteResponse* response,
		::google::protobuf::Closure* done) {
		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			int32_t ret = FileSystem::Instance().Root().QueryNode(user, request->path().c_str(), [request, response](User * user, Node * node) {
				if (node->IsDir())
					return api::master::ErrorCode::EC_IS_DIR;

				File * file = static_cast<File*>(node);
				int32_t blockCount = FileSystem::Instance().CalcBlockCount(file->GetSize());
				if (request->blockindex() < 0 || request->blockindex() >= blockCount)
					return api::master::ErrorCode::EC_OUT_OF_RANGE;

				Block * block = BlockManager::Instance().Get(BLOCK_ID(file->GetId(), request->blockindex()));
				if (!block)
					return api::master::ErrorCode::EC_BLOCK_MISSING;

				std::vector<int32_t> replicas;
				DataNode * server = block->Write(replicas);

				if (!server) {
					block->Release();
					return api::master::ErrorCode::EC_BLOCK_NOT_READY;
				}

				auto * lease = response->mutable_lease();
				lease->set_id(block->GetId());
				lease->set_until(block->GetLease());
				lease->set_version(block->GetVersion());
				lease->set_newversion(block->GetExpectVersion());

				auto * ep = lease->mutable_ep();
				ep->set_host(server->GetHost());
				ep->set_port(server->GetPort());

				for (auto r : replicas)
					lease->add_chunkservers(r);

				lease->set_key(server->CalcKey(block->GetId(), block->GetLease(), block->GetVersion(), block->GetExpectVersion()));

				block->Release();
				return api::master::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::master::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Append(::google::protobuf::RpcController* controller,
		const ::ofs::api::master::AppendRequest* request,
		::ofs::api::master::AppendResponse* response,
		::google::protobuf::Closure* done) {
		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::master::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			int32_t ret = FileSystem::Instance().Root().QueryNode(user, request->path().c_str(), [request, response](User * user, Node * node) {
				if (node->IsDir())
					return api::master::ErrorCode::EC_IS_DIR;

				File * file = static_cast<File*>(node);
				int32_t blockIndex = FileSystem::Instance().CalcAppendBlock(file->GetSize());
				Block * block = BlockManager::Instance().GetOrCreate(BLOCK_ID(file->GetId(), blockIndex));
				if (!block)
					return api::master::ErrorCode::EC_BLOCK_MISSING;

				std::vector<int32_t> replicas;
				DataNode * server = block->Write(replicas);

				if (!server) {
					block->Release();
					return api::master::ErrorCode::EC_BLOCK_NOT_READY;
				}

				auto * lease = response->mutable_lease();
				lease->set_id(block->GetId());
				lease->set_until(block->GetLease());
				lease->set_version(block->GetVersion());
				lease->set_newversion(block->GetExpectVersion());

				auto * ep = lease->mutable_ep();
				ep->set_host(server->GetHost());
				ep->set_port(server->GetPort());

				for (auto r : replicas)
					lease->add_chunkservers(r);

				lease->set_key(server->CalcKey(block->GetId(), block->GetLease(), block->GetVersion(), block->GetExpectVersion()));

				block->Release();
				return api::master::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::master::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}
}
