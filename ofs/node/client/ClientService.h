#ifndef __CLIENT_SERVICE_H__
#define __CLIENT_SERVICE_H__
#include "hnet.h"
#include "api/OfsChunk.pb.h"
#include "singleton.h"
#include "rpc/Rpc.h"
#include "XmlReader.h"

namespace ofs {
	class ClientService : public api::OfsNodeService, public olib::Singleton<ClientService> {
	public:
		ClientService() {}
		~ClientService() {}

		bool Start(const olib::IXmlObject& root);

		virtual void Read(::google::protobuf::RpcController* controller,
			const ::ofs::api::ReadRequest* request,
			::ofs::api::ReadResponse* response,
			::google::protobuf::Closure* done);

	private:
		rpc::OfsRpcServer _rpc;
	};
}

#endif //__CLIENT_SERVICE_H__
