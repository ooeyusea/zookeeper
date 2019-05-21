#ifndef __NODE_SERVICE_H__
#define __NODE_SERVICE_H__
#include "hnet.h"
#include "singleton.h"
#include "message_queue/MessageQueue.h"
#include "XmlReader.h"

namespace ofs {
	class NodeService : public olib::Singleton<NodeService> {
	public:
		NodeService() {}
		~NodeService() {}

		bool Start(const olib::IXmlObject& root);

	private:
		mq::MessageQueue * _queue = nullptr;
	};
}

#endif //__NODE_SERVICE_H__
