#include "ClientService.h"
#include "file/FileSystem.h"
#include "user/UserManager.h"
#include "chunk/ChunkServer.h"
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
		const ::ofs::api::LoginReq* request,
		::ofs::api::LoginResponse* response,
		::google::protobuf::Closure* done) {

		std::string token = UserManager::Instance().Login(request->name(), request->password());
		if (!token.empty()) {
			response->set_errcode(api::ErrorCode::EC_NONE);
			response->set_token(token);
		}
		else
			response->set_errcode(api::ErrorCode::EC_USER_OR_PASSWORD_ERROR);
	}

	void ClientService::MakeDir(::google::protobuf::RpcController* controller,
		const ::ofs::api::MakeDirRequest* request,
		::ofs::api::MakeDirResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->directory().c_str(), request->name().c_str(), request->authority(), true);
			response->set_errcode((api::ErrorCode)ret);
			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Create(::google::protobuf::RpcController* controller,
		const ::ofs::api::CreateFileRequest* request,
		::ofs::api::CreateFileResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->directory().c_str(), request->name().c_str(), request->authority(), false);
			response->set_errcode((api::ErrorCode)ret);
			UserManager::Instance().Release(user);
		}
	}

	void ClientService::List(::google::protobuf::RpcController* controller,
		const ::ofs::api::ListRequest* request,
		::ofs::api::ListResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			std::vector<Node*> ret = FileSystem::Instance().Root().List(user, request->path().c_str());

			response->set_errcode(api::ErrorCode::EC_NONE);
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
		const ::ofs::api::RemoveRequest* request,
		::ofs::api::RemoveResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			int32_t ret = FileSystem::Instance().Root().Remove(user, request->path().c_str());
			response->set_errcode((api::ErrorCode)ret);
			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Status(::google::protobuf::RpcController* controller,
		const ::ofs::api::FileStatusRequest* request,
		::ofs::api::FileStatusRespone* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
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

				return api::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Read(::google::protobuf::RpcController* controller,
		const ::ofs::api::ReadRequest* request,
		::ofs::api::ReadResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			int32_t ret = FileSystem::Instance().Root().QueryNode(user, request->path().c_str(), [request, response](User * user, Node * node) {
				if (node->IsDir())
					return api::ErrorCode::EC_IS_DIR;

				File * file = static_cast<File*>(node);
				int32_t blockCount = FileSystem::Instance().CalcBlockCount(file->GetSize());
				if (request->blockindex() < 0 || request->blockindex() >= blockCount)
					return api::ErrorCode::EC_OUT_OF_RANGE;

				Block * block = BlockManager::Instance().Get(BLOCK_ID(file->GetId(), request->blockindex()));
				if (!block)
					return api::ErrorCode::EC_BLOCK_MISSING;

				response->set_id(block->GetId());

				block->Read([response](ChunkServer * server){
					auto * ep = response->add_eps();
					ep->set_host(server->GetHost());
					ep->set_port(server->GetPort());
				});

				block->Release();
				return api::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Write(::google::protobuf::RpcController* controller,
		const ::ofs::api::WriteRequest* request,
		::ofs::api::WriteResponse* response,
		::google::protobuf::Closure* done) {
		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			int32_t ret = FileSystem::Instance().Root().QueryNode(user, request->path().c_str(), [request, response](User * user, Node * node) {
				if (node->IsDir())
					return api::ErrorCode::EC_IS_DIR;

				File * file = static_cast<File*>(node);
				int32_t blockCount = FileSystem::Instance().CalcBlockCount(file->GetSize());
				if (request->blockindex() < 0 || request->blockindex() >= blockCount)
					return api::ErrorCode::EC_OUT_OF_RANGE;

				Block * block = BlockManager::Instance().Get(BLOCK_ID(file->GetId(), request->blockindex()));
				if (!block)
					return api::ErrorCode::EC_BLOCK_MISSING;

				response->set_id(block->GetId());

				ChunkServer * server = nullptr;
				std::vector<int32_t> replicas;
				std::tie(server, replicas) = block->Write();

				if (!server) {
					block->Release();
					return api::ErrorCode::EC_BLOCK_NOT_READY;
				}

				auto * ep = response->mutable_ep();
				ep->set_host(server->GetHost());
				ep->set_port(server->GetPort());

				for (auto r : replicas)
					response->add_chunkservers(r);

				block->Release();
				return api::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Append(::google::protobuf::RpcController* controller,
		const ::ofs::api::AppendRequest* request,
		::ofs::api::AppendResponse* response,
		::google::protobuf::Closure* done) {
		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			int32_t ret = FileSystem::Instance().Root().QueryNode(user, request->path().c_str(), [request, response](User * user, Node * node) {
				if (node->IsDir())
					return api::ErrorCode::EC_IS_DIR;

				File * file = static_cast<File*>(node);
				int32_t blockIndex = FileSystem::Instance().CalcAppendBlock(file->GetSize());
				Block * block = BlockManager::Instance().GetOrCreate(BLOCK_ID(file->GetId(), blockIndex));
				if (!block)
					return api::ErrorCode::EC_BLOCK_MISSING;

				response->set_id(block->GetId());

				ChunkServer * server = nullptr;
				std::vector<int32_t> replicas;
				std::tie(server, replicas) = block->Write();

				if (!server) {
					block->Release();
					return api::ErrorCode::EC_BLOCK_NOT_READY;
				}

				auto * ep = response->mutable_ep();
				ep->set_host(server->GetHost());
				ep->set_port(server->GetPort());

				for (auto r : replicas)
					response->add_chunkservers(r);

				block->Release();
				return api::ErrorCode::EC_NONE;
			});

			response->set_errcode((api::ErrorCode)ret);

			UserManager::Instance().Release(user);
		}
	}
}
