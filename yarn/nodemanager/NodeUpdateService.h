#ifndef  __NODE_UPDATE_SERVICE_H__
#define __NODE_UPDATE_SERVICE_H__
#include "hnet.h"
#include "rpc/Rpc.h"
#include "proto/NMProtocol.pb.h"

namespace yarn {
	class YarnConfiguration;
	class NodeManager;

	class NodeUpdateService {
	public:
		NodeUpdateService(NodeManager& nm) : _nm(nm) {}
		~NodeUpdateService() {}

		void Start(const YarnConfiguration& config);

	private:
		void SetupVariable(const YarnConfiguration& config);
		void HeartBeat();

	private:
		NodeManager& _nm;
		
		rpc::YarnRpcChannel _channel;
		proto::ResourceTrackerService * _service = nullptr;

		hn_ticker * _ticker;
	};
}

#endif //__NODE_UPDATE_SERVICE_H__
