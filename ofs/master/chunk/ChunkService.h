#ifndef __CHUNK_SERVICE_H__
#define __CHUNK_SERVICE_H__
#include "hnet.h"
#include "proto/Chunk.pb.h"
#include "singleton.h"
#include "rpc/Rpc.h"
#include "XmlReader.h"

#define MAX_SERVER 65536
namespace ofs {
	class ChunkServer;
	class ChunkService : public c2m::OfsChunkService, public olib::Singleton<ChunkService> {
	public:
		ChunkService() {
			memset(_servers, 0, sizeof(_servers));
		}
		~ChunkService() {}

		bool Start(const olib::IXmlObject& root);

		virtual void RegisterChunkServer(::google::protobuf::RpcController* controller,
			const ::ofs::c2m::RegisterChunkServerRequest* request,
			::ofs::c2m::RegisterChunkServerResponse* response,
			::google::protobuf::Closure* done);
		virtual void Report(::google::protobuf::RpcController* controller,
			const ::ofs::c2m::ReportRequest* request,
			::ofs::c2m::ReportResponse* response,
			::google::protobuf::Closure* done);
		virtual void RenewLease(::google::protobuf::RpcController* controller,
			const ::ofs::c2m::RenewLeaseRequest* request,
			::ofs::c2m::RenewLeaseResponse* response,
			::google::protobuf::Closure* done);

		std::vector<ChunkServer*> Distribute(int32_t count);

	private:
		rpc::OfsRpcServer _rpc;

		ChunkServer * _servers[MAX_SERVER];
	};
}

#endif //__CLIENT_SERVICE_H__
