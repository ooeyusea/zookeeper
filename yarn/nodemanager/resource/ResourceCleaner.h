#ifndef __RESOURCE_CLEANER_H__
#define __RESOURCE_CLEANER_H__
#include "hnet.h"

namespace yarn {
	class Resource;
	class ResourceCleaner {
	public:
		ResourceCleaner(Resource * resource) : _resource(resource) {}
		~ResourceCleaner() {}

		void Start();

	private:
		Resource * _resource;
	};
}

#endif //__RESOURCE_CLEANER_H__
