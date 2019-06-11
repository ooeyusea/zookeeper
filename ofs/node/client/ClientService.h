#ifndef __CLIENT_SERVICE_H__
#define __CLIENT_SERVICE_H__
#include "hnet.h"
#include "api/OfsChunk.pb.h"
#include "singleton.h"
#include "rpc/Rpc.h"
#include "XmlReader.h"

namespace ofs {
	class ClientService : public api::chunk::OfsNodeService, public olib::Singleton<ClientService> {
	public:
		ClientService() {}
		~ClientService() {}

		bool Start(const olib::IXmlObject& root);

		virtual void Read(::google::protobuf::RpcController* controller,
			const ::ofs::api::chunk::ReadRequest* request,
			::ofs::api::chunk::ReadResponse* response,
			::google::protobuf::Closure* done);

		virtual void Write(::google::protobuf::RpcController* controller,
			const ::ofs::api::chunk::WriteRequest* request,
			::ofs::api::chunk::WriteResponse* response,
			::google::protobuf::Closure* done);

		virtual void Append(::google::protobuf::RpcController* controller,
			const ::ofs::api::chunk::AppendRequest* request,
			::ofs::api::chunk::AppendResponse* response,
			::google::protobuf::Closure* done);

		inline const std::string& GetHost() const { return _host; }
		inline int32_t GetPort() const { return _port; }

	private:
		rpc::OfsRpcServer _rpc;

		std::string _host;
		int32_t _port = 0;
	};
}

#endif //__CLIENT_SERVICE_H__
