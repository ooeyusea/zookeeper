#include "AMService.h"
#include "Configuration.h"

namespace yarn {
	bool AMService::Start(const YarnConfiguration & config) {
		if (!_server.Start(config.GetNodeManager().GetIp(), config.GetNodeManager().GetPort())) {
			hn_error("start am service rpc failed");
			return false;
		}
		return true;
	}

	void AMService::StartContainer(::google::protobuf::RpcController* controller,
		const ::yarn::api::StartContainerRequest* request,
		::yarn::api::StartContainerResponse* response,
		::google::protobuf::Closure* done) {



		if (done)
			done->Run();
	}

	void AMService::StopContainer(::google::protobuf::RpcController* controller,
		const ::yarn::api::StopContainerRequest* request,
		::yarn::api::StopContainerResponse* response,
		::google::protobuf::Closure* done) {



		if (done)
			done->Run();
	}

	void AMService::GetContainerStatus(::google::protobuf::RpcController* controller,
		const ::yarn::api::GetContainerStatusRequest* request,
		::yarn::api::GetContainerStatusResponse* response,
		::google::protobuf::Closure* done) {



		if (done)
			done->Run();
	}
}
