#ifndef __DATA_NODE_SERVICE_H__
#define __DATA_NODE_SERVICE_H__
#include "hnet.h"
#include "proto/Chunk.pb.h"
#include "singleton.h"
#include "XmlReader.h"
#include "message_queue/MessageQueue.h"

namespace ofs {
	class DataNode;
	struct IDataCluster;
	class DataNodeService : public olib::Singleton<DataNodeService> {
	public:
		DataNodeService() : _mutex(true) {}
		~DataNodeService() {}

		bool Start(const olib::IXmlObject& root);

		std::vector<DataNode*> Distribute(const std::vector<DataNode*>& old);

		inline mq::MessageQueue * GetSender() const { return _queue; }

	private:
		void OnRegister(const c2m::Register& req);

		void OnReport(const c2m::ReportBlock& req);
		void OnUpdate(const c2m::UpdataBlock& req);
		void OnClean(const c2m::CleanComplete& ntf);

		void OnHeartbeat(const c2m::Heartbeat& ntf);

	private:
		mq::MessageQueue * _queue = nullptr;

		hn_shared_mutex _mutex;
		IDataCluster * _dataCluster;
	};
}

#endif //__DATA_NODE_SERVICE_H__
