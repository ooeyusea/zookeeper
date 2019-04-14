#ifndef  __NODE_UPDATE_SERVICE_H__
#define __NODE_UPDATE_SERVICE_H__
#include "hnet.h"
#include "rpc/Rpc.h"

namespace yarn {
	class YarnConfiguration;

	class NodeUpdateService {
	public:
		NodeUpdateService() {}
		~NodeUpdateService() {}

		bool Start(YarnConfiguration& config);

	private:

	};
}

#endif //__NODE_UPDATE_SERVICE_H__
