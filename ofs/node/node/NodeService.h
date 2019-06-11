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

		void Register();
		void Write(const ::ofs::api::chunk::BlockLease& lease, int64_t oldVersion, int64_t newVersion, int32_t offset, const std::string& data);
		void Append(const ::ofs::api::chunk::BlockLease& lease, int64_t oldVersion, int64_t newVersion, const std::string& data);
		void Report(Block * block);
		void ReportClean(int64_t blockId);
		void ReportBlocks(c2m::ReportBlock& report);

	private:
		void OnWrite(const c2m::WriteNotify& ntf);
		void OnAppend(const c2m::AppendNotify& ntf);

		void OnNeighborNotify(const c2m::NeighborNotify& gossip);
		void OnNeighborGossip(const c2m::NeighborGossip& gossip);

		void OnRecoverBlock(const c2m::RecoverBlock& cmd);
		void OnCleanBlock(const c2m::CleanBlock& cmd);

		void StartHeartbeat(int64_t heartBeat);

	private:
		mq::MessageQueue * _queue = nullptr;
		int32_t _id = 0;
		std::string _host;
		int32_t _port = 0;
		int32_t _rack = 0;
		int32_t _dc = 0;
		std::string _extend;

		hn_ticker * _heartBeatTimer = nullptr;
	};
}

#endif //__NODE_SERVICE_H__
