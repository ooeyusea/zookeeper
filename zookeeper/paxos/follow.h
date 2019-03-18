#ifndef __FOLLOW_H__
#define __FOLLOW_H__
#include "hnet.h"
#include "util.h"
#include <string>
#include "DataSet.h"
#include "service.h"

class Follow : public IServiceExecutor {
public:
	Follow(int32_t id, DataSet& dataset);
	~Follow();

	int32_t Following(int32_t peerEpoch, const std::string& leaderIp, int32_t leaderPort, int32_t servicePort);

	virtual void Propose(std::string && data, const std::function<void(bool)>& fn);
	virtual void Read(std::string && data, std::string& result);

private:
	void RegisterToLeader(int32_t& peerEpoch);
	void SyncWithLeader();
	void ProcossPropose();

private:
	int32_t _id;
	DataSet& _dataset;

	int32_t _fd;
	ServiceProvider _service;

	hn_mutex _lock;
	std::unordered_map<int64_t, std::function<void(bool)>> _callbacks;
	int64_t _nextRequestId = 1;
};

#endif //__FOLLOW_H__
