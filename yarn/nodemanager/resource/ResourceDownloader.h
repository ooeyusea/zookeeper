#ifndef __RESOURCE_DOWNLOADER_H__
#define __RESOURCE_DOWNLOADER_H__
#include "hnet.h"

namespace yarn {
	class Resource;
	class ResourceDownloader {
	public:
		ResourceDownloader(Resource * resource) : _resource(resource) {}
		~ResourceDownloader() {}

		void Start();

	private:
		Resource * _resource;
	};
}

#endif //__RESOURCE_DOWNLOADER_H__
