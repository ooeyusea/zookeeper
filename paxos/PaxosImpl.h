#ifndef __PAXOSIMPL_H__
#define __PAXOSIMPL_H__
#include "hnet.h"
#include "util.h"
#include "paxos.h"
#include "dataset.h"
#include "election.h"

namespace paxos {
	class PaxosImpl : public IPaxosImpl {
	public:
		PaxosImpl(IStateData * data);
		~PaxosImpl();

		virtual bool Start(const std::string& path);

		virtual void RegisterLogType(ITransaction * transaction);
		virtual void DoTransaction(ITransaction * transaction, const char * param, int32_t size);

	private:
		int8_t _state = Election::LOOKING;
		int32_t _id = 0;
		int32_t _clientPort = 0;
		std::string _ip;
		int32_t _electionPort = 0;
		int32_t _votePort = 0;
		int32_t _servicePort = 0;

		std::vector<Server> _servers;
		Server * _leader = nullptr;

		Election _election;

		int32_t _peerEpoch = 1;
		DataSet * _dataset = nullptr;
	};
}

#endif //__ELECTION_H__
