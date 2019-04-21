#ifndef __CLIENT_SERVICE_H__
#define __CLIENT_SERVICE_H__
#include "hnet.h"
#include "api/OfsMaster.pb.h"
#include "singleton.h"
#include "rpc/Rpc.h"
#include "XmlReader.h"

namespace ofs {
	class ClientService : public api::OfsFileService, public olib::Singleton<ClientService> {
	public:
		ClientService() {}
		~ClientService() {}

		bool Start(const olib::IXmlObject& root);

		virtual void Login(::google::protobuf::RpcController* controller,
			const ::ofs::api::LoginReq* request,
			::ofs::api::LoginResponse* response,
			::google::protobuf::Closure* done);
		virtual void MakeDir(::google::protobuf::RpcController* controller,
			const ::ofs::api::MakeDirRequest* request,
			::ofs::api::MakeDirResponse* response,
			::google::protobuf::Closure* done);
		virtual void CreateFile(::google::protobuf::RpcController* controller,
			const ::ofs::api::CreateFileRequest* request,
			::ofs::api::CreateFileResponse* response,
			::google::protobuf::Closure* done);
		virtual void List(::google::protobuf::RpcController* controller,
			const ::ofs::api::ListRequest* request,
			::ofs::api::ListResponse* response,
			::google::protobuf::Closure* done);
		virtual void Remove(::google::protobuf::RpcController* controller,
			const ::ofs::api::RemoveRequest* request,
			::ofs::api::RemoveResponse* response,
			::google::protobuf::Closure* done);
		virtual void Open(::google::protobuf::RpcController* controller,
			const ::ofs::api::OpenRequest* request,
			::ofs::api::OpenResponse* response,
			::google::protobuf::Closure* done);
		virtual void Close(::google::protobuf::RpcController* controller,
			const ::ofs::api::CloseRequest* request,
			::ofs::api::CloseResponse* response,
			::google::protobuf::Closure* done);

	private:
		rpc::OfsRpcServer _rpc;
	};
}

#endif //__CLIENT_SERVICE_H__