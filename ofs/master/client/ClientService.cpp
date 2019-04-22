#include "ClientService.h"
#include "file/FileSystem.h"
#include "user/UserManager.h"

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
			int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->path().c_str(), request->authority(), true);
			response->set_errcode((api::ErrorCode)ret);
			UserManager::Instance().Release(user);
		}
	}

	void ClientService::CreateFile(::google::protobuf::RpcController* controller,
		const ::ofs::api::CreateFileRequest* request,
		::ofs::api::CreateFileResponse* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->path().c_str(), request->authority(), false);
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
		const ::ofs::api::FileStatusRespone* request,
		::ofs::api::FileStatusRespone* response,
		::google::protobuf::Closure* done) {

		User * user = UserManager::Instance().Acquire(request->token());
		if (!user)
			response->set_errcode(api::ErrorCode::EC_USER_EXPIRE);
		else {
			// because node is not delete immediately
			Node* node = FileSystem::Instance().Root().QueryNode(user, request->path().c_str());
			if (node) {
				response->set_errcode(api::ErrorCode::EC_NONE);
				auto * n = response->mutable_file();

				n->set_name(node->GetName());
				n->set_owner(node->GetOwner());
				n->set_group(node->GetOwnerGroup());
				n->set_authority(node->GetAuthority());
				n->set_size(node->GetSize());
				n->set_createtime(node->GetCreateTime());
				n->set_updatetime(node->GetUpdateTime());
				n->set_dir(node->IsDir());
			}
			else {
				response->set_errcode(api::ErrorCode::EC_PERMISSION_DENY);
			}

			UserManager::Instance().Release(user);
		}
	}

	void ClientService::Read(::google::protobuf::RpcController* controller,
		const ::ofs::api::ReadRequest* request,
		::ofs::api::ReadResponse* response,
		::google::protobuf::Closure* done) {

	}

	void ClientService::Write(::google::protobuf::RpcController* controller,
		const ::ofs::api::WriteRequest* request,
		::ofs::api::WriteResponse* response,
		::google::protobuf::Closure* done) {

	}
}
