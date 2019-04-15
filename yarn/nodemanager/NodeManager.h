#ifndef __NODEMANAGER_H__
#define __NODEMANAGER_H__
#include "hnet.h"
#include "api/AMNodeManager.pb.h"
#include "rpc/Rpc.h"
#include "event/impl/AsyncEventDispatcher.h"
#include "ContainerManager.h"
#include "NodeUpdateService.h"
#include "AMService.h"
#include "Configuration.h"

namespace yarn {
	class NodeManager {
	public:
		NodeManager() : _config(false), _containerManager(*this), _nodeUpdateService(*this), _amService(*this) {}
		virtual ~NodeManager() {}

		bool Start(const char * path);

		inline const YarnConfiguration& GetConfiguration() const { return _config; }
		inline ContainerManager& GetContainerManager() { return _containerManager; }
		inline NodeUpdateService& GetNodeUpdateService() { return _nodeUpdateService; }
		inline AMService& GetAMService() { return _amService; }

		inline event::AsyncEventDispatcher& GetDispatcher() { return _dispatcher; }

	private:
		event::AsyncEventDispatcher _dispatcher;
		
		YarnConfiguration _config;
		ContainerManager _containerManager;
		NodeUpdateService _nodeUpdateService;
		AMService _amService;
	};
}

#endif //__NODEMANAGER_H__
