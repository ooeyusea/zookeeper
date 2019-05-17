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

	private:
		rpc::OfsRpcServer _rpc;
	};
}

#endif //__CLIENT_SERVICE_H__
