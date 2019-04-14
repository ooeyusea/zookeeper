#include "NodeManager.h"
#include "Configuration.h"

namespace yarn {
	bool NodeManager::Start(const YarnConfiguration & config) {
		_name = config.GetNodeManager().GetName();
		_ip = config.GetNodeManager().GetIp();
		_port = config.GetNodeManager().GetPort();

		if (!_nodeUpdateService.Start(config)) {
			hn_error("start rpc server failed");
			return false;
		}

		_containerManager.AddTo(_dispatcher);
		_server.AddService(_containerManager);
		return _server.Start(_ip, _port);
	}
}
