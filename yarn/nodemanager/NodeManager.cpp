#include "NodeManager.h"
#include "Configuration.h"
#include "ContainerManager.h"
#include "AMService.h"
#include "NodeUpdateService.h"

namespace yarn {
	bool NodeManager::Start(const char * path) {
		if (!_config.LoadFrom(path))
			return false;
		hn_info("load configuration success");

		if (!AMService::Instance().Start(_config))
			return false;
		hn_info("start am service success");

		NodeUpdateService::Instance().Start(_config);
		hn_info("start node heart-beat service success");

		ContainerManager::Instance().Start(_config);
		hn_info("start container manager success");
		return true;
	}
}
