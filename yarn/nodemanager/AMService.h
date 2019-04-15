#ifndef __AMSERVICE_H__
#define __AMSERVICE_H__
#include "hnet.h"
#include "api/AMNodeManager.pb.h"
#include "proto/NMProtocol.pb.h"
#include "rpc/Rpc.h"

namespace yarn {
	class NodeManager;
	class YarnConfiguration;
	class AMService : public api::ContainerManagementService {
	public:
		AMService(NodeManager& nm) : _nm(nm) {}
		~AMService() {}

		bool Start(const YarnConfiguration & config);

		virtual void StartContainer(::google::protobuf::RpcController* controller,
			const ::yarn::api::StartContainerRequest* request,
			::yarn::api::StartContainerResponse* response,
			::google::protobuf::Closure* done);
		virtual void StopContainer(::google::protobuf::RpcController* controller,
			const ::yarn::api::StopContainerRequest* request,
			::yarn::api::StopContainerResponse* response,
			::google::protobuf::Closure* done);
		virtual void GetContainerStatus(::google::protobuf::RpcController* controller,
			const ::yarn::api::GetContainerStatusRequest* request,
			::yarn::api::GetContainerStatusResponse* response,
			::google::protobuf::Closure* done);

	private:
		NodeManager& _nm;

		rpc::YarnRpcServer _server;
	};
}

#endif //__AMSERVICE_H__
