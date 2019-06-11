#include "OfsMaster.h"
#include "file/FileSystem.h"
#include "user/UserManager.h"
#include "client/ClientService.h"
#include "chunk/DataNodeService.h"
#include "XmlReader.h"

namespace ofs {
	Master::Master() {

	}

	bool Master::Start(const std::string& path) {
		olib::XmlReader conf;
		if (!conf.LoadXml(path.c_str())) {
			hn_error("load config failed: {}", path);
			return false;
		}

		try {
			if (!UserManager::Instance().Start(conf.Root()))
				return false;
			
			hn_info("start user manater success");

			if (!FileSystem::Instance().Start(conf.Root()))
				return false;

			hn_info("load file system success");

			if (!ClientService::Instance().Start(conf.Root()))
				return false;

			hn_info("start client service success");

			if (!DataNodeService::Instance().Start(conf.Root()))
				return false;

			hn_info("start data node service success");
		}
		catch (std::exception& e) {
			hn_error("load config format error: {} {}", e.what(), path);
			return false;
		}

		return true;
	}
}
