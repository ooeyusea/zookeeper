#ifndef __FOLLOW_H__
#define __FOLLOW_H__
#include "hnet.h"
#include "util.h"
#include <string>
#include "DataSet.h"
#include "service.h"

namespace paxos {
	class PaxosImpl;
	class Follow : public ServiceExecutor {
	public:
		Follow(int32_t id, DataSet& dataset);
		~Follow();

		int32_t Following(int32_t peerEpoch, const std::string& leaderIp, int32_t leaderPort, PaxosImpl * impl);

		virtual void Propose(std::string && data, ITransaction * transaction);

	private:
		void RegisterToLeader(int32_t& peerEpoch);
		void SyncWithLeader();
		void ProcossPropose();

	private:
		int32_t _id;
		DataSet& _dataset;

		int32_t _fd;
	};
}
#endif //__FOLLOW_H__
