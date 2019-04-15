#ifndef __CONTAINER_EVENT_H__
#define __CONTAINER_EVENT_H__
#include "hnet.h"
#include "event/EventDispatcher.h"

namespace yarn {
	namespace event {
		enum class ContainerEventType : int32_t {
			CET_INIT,

			CET_CLEANUP,
		};

		struct InitContainer {
			std::string name;
			int32_t cpu;
			int32_t res;
			int32_t virt;
			int32_t disk;

			InitContainer(const std::string& name_, int32_t cpu_, int32_t res_, int32_t virt_, int32_t disk_)
				: name(name_), cpu(cpu_), res(res_), virt(virt_), disk(disk_) {}
		};

		typedef Event<ContainerEventType, ContainerEventType::CET_INIT, InitContainer> InitContainerEvent;

		struct CleanupContainer {
			std::string name;

			CleanupContainer(const std::string& name_)
				: name(name_) {}
		};

		typedef Event<ContainerEventType, ContainerEventType::CET_CLEANUP, CleanupContainer> CleanupContainerEvent;
	}
}

#endif //__CONTAINER_EVENT_H__
