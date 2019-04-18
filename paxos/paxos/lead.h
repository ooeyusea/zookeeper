#ifndef __LEAD_H__
#define __LEAD_H__
#include "hnet.h"
#include "util.h"
#include <string>
#include "DataSet.h"
#include "define.h"
#include "service.h"

namespace paxos {
	class Lead : public IServiceExecutor {
		struct Draft {
			int32_t id;
			int64_t requestId;
			int64_t zxId;
			std::string data;
			std::vector<bool> vote;
		};

		struct TransactionCall {
			ITransaction * transaction;
			hn_co co;
		};

	public:
		Lead(int32_t id, DataSet& dataset, int32_t serverCount);
		~Lead();

		int32_t Leading(int32_t peerEpoch, int32_t votePort);

		virtual void Propose(std::string && data, ITransaction * transaction);
		std::tuple<ITransaction*, hn_co> PopRequest(int64_t requestId);

		void Propose(int32_t id, int64_t requestId, std::string && data);
		void AckPropose(int32_t id, int64_t zxId);

		int32_t GetPeerEpoch(int32_t id, int32_t peerEpoch);
		void WaitForNewLeader(int32_t id);

		inline void Register(int32_t id, int32_t fd) {
			if (_followers[id - 1] > 0) {
				throw std::logic_error("already has follower ");
			}

			_followers[id - 1] = fd;
		}

		inline void Unregister(int32_t id, int32_t fd) {
			_followers[id - 1] = 0;

			if (_forwarding[id - 1] > 0)
				_forwarding[id - 1] = 0;
		}

		inline void StartForwarding(int32_t id, int32_t fd) {
			_forwarding[id - 1] = fd;
		}

	private:
		void Process();
		bool LaunchNextDraft(std::unique_lock<hn_mutex>& guard);

		bool StartListenFollow(int32_t votePort);
		void ShutdownListen();

	private:
		int32_t _fd;
		bool _terminate = false;

		int32_t _id;
		int32_t _serverCount;
		DataSet& _dataset;

		std::vector<int32_t> _followers;
		std::vector<int32_t> _forwarding;
		std::atomic<int32_t> _count;

		bool _waitForPeerEpoch = true;
		int32_t _peerEpoch = 0;
		std::vector<int32_t> _recvPeerEpoch;
		std::vector<bool> _recvNewLeader;

		hn_mutex _cbLock;
		std::unordered_map<int64_t, TransactionCall> _callbacks;
		int64_t _nextRequestId = 1;

		hn_mutex _lock;
		std::map<int64_t, Draft> _uncommit;

		int64_t _nextId = 1;
	};
}

#endif //__LEAD_H__
