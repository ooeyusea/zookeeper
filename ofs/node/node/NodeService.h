#ifndef __NODE_SERVICE_H__
#define __NODE_SERVICE_H__
#include "hnet.h"
#include "singleton.h"
#include "message_queue/MessageQueue.h"
#include "XmlReader.h"
#include "proto/Chunk.pb.h"

namespace ofs {
#define MASTER_NODE 0
	namespace api {
		namespace chunk {
			class BlockLease;
		}
	}

	class Block;
	class NodeService : public olib::Singleton<NodeService> {
	public:
		NodeService() {}
		~NodeService() {}

		bool Start(const olib::IXmlObject& root);

		void Write(const ::ofs::api::chunk::BlockLease& lease, int64_t oldVersion, int64_t newVersion, int32_t offset, const std::string& data);
		void Append(const ::ofs::api::chunk::BlockLease& lease, int64_t oldVersion, int64_t newVersion, const std::string& data);
		void Report(Block * block);

	private:
		void OnWrite(const c2m::WriteNotify& ntf);
		void OnAppend(const c2m::AppendNotify& ntf);

	private:
		mq::MessageQueue * _queue = nullptr;
		int32_t _id;
	};
}

#endif //__NODE_SERVICE_H__
