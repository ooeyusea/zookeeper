#include "IdleDetectService.h"
#include <fstream>

namespace ofs {
	bool IdleDetectService::Start(const olib::IXmlObject& root) {
		_path = root["node"][0]["idle"][0].GetAttributeString("path");
		std::ifstream in(_path);
		if (in) {
			in >> _idleStart >> _idleEnd;
			if (in) {
				hn_info("idle detect service read file {} <-> {}", _idleStart, _idleEnd);
				return true;
			}
		}

		_idleStart = root["node"][0]["idle"][0].GetAttributeInt32("start");
		_idleEnd = root["node"][0]["idle"][0].GetAttributeInt32("end");
		hn_info("idle detect service read xml {} <-> {}", _idleStart, _idleEnd);
		return true;
	}

	bool IdleDetectService::IsIdle() {

		return false;
	}

}
