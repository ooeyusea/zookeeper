#ifndef __LEAD_H__
#define __LEAD_H__
#include "hnet.h"
#include "util.h"
#include <string>
#include "DataSet.h"
#include "define.h"

class Lead {
	struct Message {
		int32_t id;
		int8_t type;
		std::string data;
	};
public:
	Lead();
	~Lead();

	int32_t Leading(int32_t id, int32_t peerEpoch, DataSet& dataset, int32_t votePort, int32_t serverCount);

	void Propose(int32_t id, const std::string& data);
	void AckPropose(int32_t id, int64_t zxId, DataSet& dataset);

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
		_forwarding[id - 1] = 0;
	}

private:
	void ProcessRequest(int32_t id, DataSet& dataset, int32_t serverCount, int32_t peerEpoch);

	bool StartListenFollow(int32_t id, int32_t serverCount, int32_t votePort, DataSet& dataset);
	void ShutdownListen();

	void SendLeaderInfo(int32_t id, int32_t peerEpoch);
	void Diff(int32_t id, DataSet& dataset, int64_t lastZxId);
	void Trunc(int32_t id, DataSet& dataset, int64_t lastZxId);
	void Snap(int32_t id, DataSet& dataset);
	void Updated(int32_t id, DataSet& dataset);

private:
	hn_channel<Message, 100> _messages;
	int32_t _fd;
	bool _terminate = false;

	std::vector<int32_t> _followers;
	std::vector<int32_t> _forwarding;
	std::atomic<int32_t> _count;

	bool _waitForPeerEpoch = true;
	int32_t peerEpoch;
	std::vector<int32_t> _recvPeerEpoch;
	std::vector<bool> _recvNewLeader;
};

#endif //__LEAD_H__
