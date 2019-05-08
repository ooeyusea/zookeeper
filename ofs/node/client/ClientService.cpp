#include "ClientService.h"

namespace ofs {
	bool ClientService::Start(const olib::IXmlObject& root) {
		_rpc.AddService(this);

		const char * host = root["client"][0].GetAttributeString("host");
		int32_t port = root["client"][0].GetAttributeInt32("port");
		return _rpc.Start(host, port);
	}
	void ClientService::Read(::google::protobuf::RpcController* controller,
		const ::ofs::api::ReadRequest* request,
		::ofs::api::ReadResponse* response,
		::google::protobuf::Closure* done) {

	}
}
