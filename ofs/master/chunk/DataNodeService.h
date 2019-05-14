#ifndef __DATA_NODE_SERVICE_H__
#define __DATA_NODE_SERVICE_H__
#include "hnet.h"
#include "proto/Chunk.pb.h"
#include "singleton.h"
#include "rpc/Rpc.h"
#include "XmlReader.h"

namespace ofs {
	class DataNode;
	struct IDataCluster;
	class DataNodeService : public c2m::OfsChunkService, public olib::Singleton<DataNodeService> {
	public:
		DataNodeService() : _mutex(true) {}
		~DataNodeService() {}

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

		std::vector<DataNode*> Distribute(const std::vector<DataNode*>& old);

	private:
		rpc::OfsRpcServer _rpc;

		hn_shared_mutex _mutex;
		IDataCluster * _dataCluster;
	};
}

#endif //__DATA_NODE_SERVICE_H__
