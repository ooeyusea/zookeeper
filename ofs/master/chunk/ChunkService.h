#ifndef __CHUNK_SERVICE_H__
#define __CHUNK_SERVICE_H__
#include "hnet.h"
#include "proto/Chunk.pb.h"
#include "singleton.h"
#include "rpc/Rpc.h"
#include "XmlReader.h"

namespace ofs {
	class ChunkService : public c2m::OfsChunkService, public olib::Singleton<ChunkService> {
	public:
		ChunkService() {}
		~ChunkService() {}

		bool Start(const olib::IXmlObject& root);

		virtual void Report(::google::protobuf::RpcController* controller,
			const ::ofs::c2m::ReportRequest* request,
			::ofs::c2m::ReportResponse* response,
			::google::protobuf::Closure* done);
		virtual void AskLease(::google::protobuf::RpcController* controller,
			const ::ofs::c2m::AskLeaseRequest* request,
			::ofs::c2m::AskLeaseResponse* response,
			::google::protobuf::Closure* done);
		virtual void Copy(::google::protobuf::RpcController* controller,
			const ::ofs::c2m::CopyRequest* request,
			::ofs::c2m::CopyResponse* response,
			::google::protobuf::Closure* done);

	private:
		rpc::OfsRpcServer _rpc;
	};
}

#endif //__CLIENT_SERVICE_H__
