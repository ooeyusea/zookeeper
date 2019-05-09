#include "OfsNode.h"
#include "XmlReader.h"

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
			
		}
		catch (std::exception& e) {
			hn_error("load config format error: {} {}", e.what(), path);
			return false;
		}

		return true;
	}
}

