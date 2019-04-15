#include "NodeUpdateService.h"
#include "Configuration.h"
#include "NodeManager.h"
#include "event/ContainerEvent.h"

namespace yarn {
	void NodeUpdateService::Start(const YarnConfiguration& config) {
		SetupVariable(config);

		_channel.Start(config.GetResourceManager().GetIp(), config.GetResourceManager().GetPort(), [this] {
			proto::RegisterNMRequest request;
			request.set_name(_nm.GetConfiguration().GetNodeManager().GetName());
			request.set_ip(_nm.GetConfiguration().GetNodeManager().GetIp());
			request.set_port(_nm.GetConfiguration().GetNodeManager().GetPort());

			rpc::YarnRpcController controller;
			proto::RegisterNMResponse response;
			_service->RegisterNM(&controller, &request, &response, nullptr);
		});

		hn_fork [this]{
			HeartBeat();
		};
	}

	void NodeUpdateService::SetupVariable(const YarnConfiguration& config) {
		_service = new proto::ResourceTrackerService::Stub(&_channel);
		_ticker = new hn_ticker(config.GetNodeManager().GetHeartBeatInterval());
	}

	void NodeUpdateService::HeartBeat() {
		for (auto t : *_ticker) {
			proto::HeartBeatRequest request;
			request.set_name(_nm.GetConfiguration().GetNodeManager().GetName());

			_nm.GetContainerManager().Pack(request);

			rpc::YarnRpcController controller;
			proto::HeartBeatResponse response;
			_service->HeartBeat(&controller, &request, &response, nullptr);

			if (!controller.Failed()) {
				for (int32_t i = 0; i < response.commands_size(); ++i) {
					auto& command = response.commands(i);

					switch (command.type()) {
					case proto::ContainerCommandType::INIT: {
							
						}
						break;
					case proto::ContainerCommandType::CLEANUP: {
							
						}
						break;
					}
				}
			}
		}
	}
}
