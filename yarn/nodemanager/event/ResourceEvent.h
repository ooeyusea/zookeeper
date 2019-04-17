#ifndef __RESOURCE_EVENT_H__
#define __RESOURCE_EVENT_H__
#include "hnet.h"
#include "event/EventDispatcher.h"

namespace yarn {
	namespace event {
		enum class ResourceEventType : int32_t {
			RET_INIT,
			RET_LOCAIZED,
			RET_LOCAIZATION_FAILED,
			RET_CLEANUP,
			RET_CLEANUP_DONE,
		};


		typedef Event<ResourceEventType, ResourceEventType::RET_INIT, std::string> InitResourceEvent;
		typedef Event<ResourceEventType, ResourceEventType::RET_LOCAIZED, std::string> ResourceLocalizedEvent;
		typedef Event<ResourceEventType, ResourceEventType::RET_LOCAIZATION_FAILED, std::string> ResourceLocalizationFailedEvent;
		typedef Event<ResourceEventType, ResourceEventType::RET_CLEANUP, std::string> ResourceCleanUpEvent;
		typedef Event<ResourceEventType, ResourceEventType::RET_CLEANUP_DONE, std::string> ResourceCleanUpDoneEvent;
	}
}

#endif //__RESOURCE_EVENT_H__
