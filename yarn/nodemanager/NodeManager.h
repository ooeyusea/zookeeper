#ifndef __NODEMANAGER_H__
#define __NODEMANAGER_H__
#include "hnet.h"
#include "api/AMNodeManager.pb.h"
#include "rpc/Rpc.h"
#include "event/impl/AsyncEventDispatcher.h"

namespace yarn {
	class YarnConfiguration;
	class NodeManager {
	public:
		NodeManager() : _containerManager(*this), _nodeUpdateService(*this) {}
		virtual ~NodeManager() {}

		bool Start(const YarnConfiguration & config);

		inline ContainerManager& GetContainerManager() { return _containerManager; }
		inline NodeUpdateService& GetNodeUpdateService() { return _nodeUpdateService; }

	private:
		rpc::YarnRpcServer _server;
		rpc::YarnRpcChannel _channel;
		event::AsyncEventDispatcher _dispatcher;

		std::string _name;
		std::string _ip;
		int32_t _port;
		
		ContainerManager _containerManager;
		NodeUpdateService _nodeUpdateService;
	};
}

#endif //__NODEMANAGER_H__
