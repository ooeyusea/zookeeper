#ifndef __IDLE_DETECT_SERVICE_H__
#define __IDLE_DETECT_SERVICE_H__
#include "hnet.h"
#include "singleton.h"
#include "XmlReader.h"

namespace ofs {
	class IdleDetectService : public olib::Singleton<IdleDetectService> {
	public:
		IdleDetectService() {}
		~IdleDetectService() {}

		bool Start(const olib::IXmlObject& root);

		bool IsIdle();

	private:
		std::string _path;
		int64_t _idleStart = 0;
		int64_t _idleEnd = 0;
	};
}

#endif //__DATA_NODE_SERVICE_H__
