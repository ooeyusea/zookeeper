#ifndef __SERVICE_H__
#define __SERVICE_H__
#include "hnet.h"
#include "transaction.h"

struct IServiceExecutor {
	virtual ~IServiceExecutor() {}

	virtual void Propose(std::string && data, const std::function<void (bool)>& fn) = 0;
	virtual void Read(std::string && data, std::string& result) = 0;
};

class ServiceProvider {
public:
	ServiceProvider(IServiceExecutor & executor)
		: _executor(executor) {

	}

	~ServiceProvider() {}

	bool Start(int32_t port);
	void Stop();

private:
	void Process(int32_t fd);

private:
	IServiceExecutor & _executor;

	int32_t _fd;

	bool _terminate = false;
	std::atomic<int> _count;
};

#endif //__SERVICE_H__