#ifndef __NODEMANAGER_H__
#define __NODEMANAGER_H__
#include "hnet.h"
#include "api/AMNodeManager.pb.h"
#include "rpc/Rpc.h"
#include "event/impl/AsyncEventDispatcher.h"
#include "singleton.h"
#include "ContainerManager.h"
#include "NodeUpdateService.h"
#include "AMService.h"
#include "Configuration.h"

namespace yarn {
	class NodeManager : public Singleton<NodeManager> {
	public:
		NodeManager() : _config(false) {}
		virtual ~NodeManager() {}

		bool Start(const char * path);

		inline const YarnConfiguration& GetConfiguration() const { return _config; }
		inline event::AsyncEventDispatcher& GetDispatcher() { return _dispatcher; }

	private:
		event::AsyncEventDispatcher _dispatcher;
		
		YarnConfiguration _config;
	};
}

#endif //__NODEMANAGER_H__
