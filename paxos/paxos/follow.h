#ifndef __FOLLOW_H__
#define __FOLLOW_H__
#include "hnet.h"
#include "util.h"
#include <string>
#include "DataSet.h"
#include "service.h"

namespace paxos {
	class Follow : public IServiceExecutor {
		struct TransactionCall {
			ITransaction * transaction;
			hn_co co;
		};
	public:
		Follow(int32_t id, DataSet& dataset);
		~Follow();

		int32_t Following(int32_t peerEpoch, const std::string& leaderIp, int32_t leaderPort);

		virtual void Propose(std::string && data, ITransaction * transaction);
		std::tuple<ITransaction*, hn_co> PopRequest(int64_t requestId);

	private:
		void RegisterToLeader(int32_t& peerEpoch);
		void SyncWithLeader();
		void ProcossPropose();

	private:
		int32_t _id;
		DataSet& _dataset;

		int32_t _fd;

		hn_mutex _lock;
		std::unordered_map<int64_t, TransactionCall> _callbacks;
		int64_t _nextRequestId = 1;
	};
}
#endif //__FOLLOW_H__
