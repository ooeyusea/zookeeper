#include "ChunkService.h"


namespace ofs {
	bool ChunkService::Start(const olib::IXmlObject& root) {
		_rpc.AddService(this);

		const char * host = root["chunk_server"][0].GetAttributeString("host");
		int32_t port = root["chunk_server"][0].GetAttributeInt32("port");
		return _rpc.Start(host, port);
	}

	void ChunkService::Report(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::ReportRequest* request,
		::ofs::c2m::ReportResponse* response,
		::google::protobuf::Closure* done) {

	}

	void ChunkService::AskLease(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::AskLeaseRequest* request,
		::ofs::c2m::AskLeaseResponse* response,
		::google::protobuf::Closure* done) {

	}

	void ChunkService::Copy(::google::protobuf::RpcController* controller,
		const ::ofs::c2m::CopyRequest* request,
		::ofs::c2m::CopyResponse* response,
		::google::protobuf::Closure* done) {

	}

}
