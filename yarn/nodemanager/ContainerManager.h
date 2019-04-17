#ifndef __CONTAINER_MANAGER_H__
#define __CONTAINER_MANAGER_H__
#include "hnet.h"
#include "proto/NMProtocol.pb.h"
#include "singleton.h"

namespace yarn {
	class YarnConfiguration;
	class ContainerManager : public Singleton<ContainerManager> {
	public:
		ContainerManager() {}
		~ContainerManager() {}

		void Start(const YarnConfiguration& config);

		void Pack(proto::HeartBeatRequest& request);
	private:
	};
}

#endif //__CONTAINER_MANAGER_H__
