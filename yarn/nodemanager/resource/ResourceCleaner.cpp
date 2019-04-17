#include "ResourceCleaner.h"
#include <stdio.h>
#include "Resource.h"
#include "NodeManager.h"
#include "event/ResourceEvent.h"

namespace yarn {
	void ResourceCleaner::Start() {
		remove(_resource->GetLocal().c_str());

		NodeManager::Instance().GetDispatcher().Handle(event::ResourceCleanUpDoneEvent::Create(_resource->GetRemote()));
	}
}

