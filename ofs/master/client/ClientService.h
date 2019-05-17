#ifndef __CLIENT_SERVICE_H__
#define __CLIENT_SERVICE_H__
#include "hnet.h"
#include "api/OfsMaster.pb.h"
#include "singleton.h"
#include "rpc/Rpc.h"
#include "XmlReader.h"

namespace ofs {
	class ClientService : public api::master::OfsFileService, public olib::Singleton<ClientService> {
	public:
		ClientService() {}
		~ClientService() {}

		bool Start(const olib::IXmlObject& root);

		virtual void Login(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::LoginReq* request,
			::ofs::api::master::LoginResponse* response,
			::google::protobuf::Closure* done);
		virtual void MakeDir(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::MakeDirRequest* request,
			::ofs::api::master::MakeDirResponse* response,
			::google::protobuf::Closure* done);
		virtual void Create(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::CreateFileRequest* request,
			::ofs::api::master::CreateFileResponse* response,
			::google::protobuf::Closure* done);
		virtual void List(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::ListRequest* request,
			::ofs::api::master::ListResponse* response,
			::google::protobuf::Closure* done);
		virtual void Remove(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::RemoveRequest* request,
			::ofs::api::master::RemoveResponse* response,
			::google::protobuf::Closure* done);
		virtual void Status(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::FileStatusRequest* request,
			::ofs::api::master::FileStatusRespone* response,
			::google::protobuf::Closure* done);
		virtual void Read(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::ReadRequest* request,
			::ofs::api::master::ReadResponse* response,
			::google::protobuf::Closure* done);
		virtual void Write(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::WriteRequest* request,
			::ofs::api::master::WriteResponse* response,
			::google::protobuf::Closure* done);
		virtual void Append(::google::protobuf::RpcController* controller,
			const ::ofs::api::master::AppendRequest* request,
			::ofs::api::master::AppendResponse* response,
			::google::protobuf::Closure* done);

	private:
		rpc::OfsRpcServer _rpc;
	};
}

#endif //__CLIENT_SERVICE_H__
