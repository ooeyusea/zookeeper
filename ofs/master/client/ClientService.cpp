#include "ClientService.h"
#include "file/FileSystem.h"

namespace ofs {
	bool ClientService::Start(const olib::IXmlObject& root) {
		_rpc.AddService(this);
		return true;
	}

	void ClientService::Login(::google::protobuf::RpcController* controller,
		const ::ofs::api::LoginReq* request,
		::ofs::api::LoginResponse* response,
		::google::protobuf::Closure* done) {

	}

	void ClientService::MakeDir(::google::protobuf::RpcController* controller,
		const ::ofs::api::MakeDirRequest* request,
		::ofs::api::MakeDirResponse* response,
		::google::protobuf::Closure* done) {

		int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->path().c_str(), request->authority(), true);
		response->set_errcode((api::ErrorCode)ret);
	}

	void ClientService::CreateFile(::google::protobuf::RpcController* controller,
		const ::ofs::api::CreateFileRequest* request,
		::ofs::api::CreateFileResponse* response,
		::google::protobuf::Closure* done) {

		int32_t ret = FileSystem::Instance().Root().CreateNode(user, request->path().c_str(), request->authority(), false);
		response->set_errcode((api::ErrorCode)ret);
	}

	void ClientService::List(::google::protobuf::RpcController* controller,
		const ::ofs::api::ListRequest* request,
		::ofs::api::ListResponse* response,
		::google::protobuf::Closure* done) {

	}

	void ClientService::Remove(::google::protobuf::RpcController* controller,
		const ::ofs::api::RemoveRequest* request,
		::ofs::api::RemoveResponse* response,
		::google::protobuf::Closure* done) {

	}

	void ClientService::Open(::google::protobuf::RpcController* controller,
		const ::ofs::api::OpenRequest* request,
		::ofs::api::OpenResponse* response,
		::google::protobuf::Closure* done) {

	}

	void ClientService::Close(::google::protobuf::RpcController* controller,
		const ::ofs::api::CloseRequest* request,
		::ofs::api::CloseResponse* response,
		::google::protobuf::Closure* done) {

	}
}
