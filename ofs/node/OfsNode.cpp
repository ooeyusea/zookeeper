#include "OfsNode.h"
#include "XmlReader.h"
#include "block/BlockManager.h"
#include "client/ClientService.h"
#include "node/NodeService.h"

namespace ofs {
	Node::Node() {

	}

	bool Node::Start(const std::string& path) {
		olib::XmlReader conf;
		if (!conf.LoadXml(path.c_str())) {
			hn_error("load config failed: {}", path);
			return false;
		}

		try {
			if (!BlockManager::Instance().Start(conf.Root())) {
				hn_error("start block manager failed");
				return false;
			}

			hn_info("start block manager success");

			if (!ClientService::Instance().Start(conf.Root())) {
				hn_error("start client service failed");
				return false;
			}

			hn_info("start client service success");

			if (!NodeService::Instance().Start(conf.Root())) {
				hn_error("start node service failed");
				return false;
			}

			hn_info("start node service success");
		}
		catch (std::exception& e) {
			hn_error("load config format error: {} {}", e.what(), path);
			return false;
		}

		return true;
	}
}
