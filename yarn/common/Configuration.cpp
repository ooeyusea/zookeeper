#include "Configuration.h"

namespace yarn {
	bool YarnNodeManagerConfiguration::Initialize(const olib::IXmlObject& object) {
		_name = object["name"][0].GetAttributeString("value");
		_ip = object["address"][0].GetAttributeString("ip");
		_port = object["address"][0].GetAttributeInt32("port");

		_heartBeatInterval = object["service"][0]["heart_beart"][0].GetAttributeInt32("interval");

		return true;
	}

	bool YarResourceManagerConfiguration::Initialize(const olib::IXmlObject& object) {
		_ip = object["address"][0].GetAttributeString("ip");
		_port = object["address"][0].GetAttributeInt32("port");

		return true;
	}

	bool YarnConfiguration::LoadFrom(const char * path) {
		olib::XmlReader conf;
		if (!conf.LoadXml(path)) {
			hn_error("load configuration failed for : {}", path);
			return false;
		}

		try {
			if (!_isRm) {
				if (!_node.Initialize(conf.Root()["node_manager"][0]))
					return false;
			}

			if (!_resouce.Initialize(conf.Root()["resource_manager"][0]))
				return false;

		}
		catch (std::exception& e) {
			hn_error("load configuration occupt exception : {}", e.what());
			return false;
		}

		return true;
	}
}
