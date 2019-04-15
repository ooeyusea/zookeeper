#include "NodeManager.h"
#include "Configuration.h"

namespace yarn {
	bool NodeManager::Start(const char * path) {
		if (!_config.LoadFrom(path))
			return false;
		hn_info("load configuration success");

		if (!_amService.Start(_config))
			return false;
		hn_info("start am service success");

		_nodeUpdateService.Start(_config);
		hn_info("start node heart-beat service success");

		_containerManager.Start(_config);
		hn_info("start container manager success");
		return true;
	}
}
