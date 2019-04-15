#ifndef __CONTAINER_MANAGER_H__
#define __CONTAINER_MANAGER_H__
#include "hnet.h"
#include "proto/NMProtocol.pb.h"

namespace yarn {
	class NodeManager;
	class YarnConfiguration;
	class ContainerManager {
	public:
		ContainerManager(NodeManager& nm) : _nm(nm) {}
		~ContainerManager() {}

		void Start(const YarnConfiguration& config);

		void Pack(proto::HeartBeatRequest& request);
	private:
		NodeManager& _nm;
	};
}

#endif //__CONTAINER_MANAGER_H__
