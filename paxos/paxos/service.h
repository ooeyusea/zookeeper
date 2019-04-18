#ifndef __SERVICE_H__
#define __SERVICE_H__
#include "hnet.h"
#include "paxos.h"

namespace paxos {
	struct IServiceExecutor {
		virtual ~IServiceExecutor() {}

		virtual void Propose(std::string && data, ITransaction * ) = 0;

		bool IsActive() const { return _active; }
	protected:
		bool _active = false;
	};
}

#endif //__SERVICE_H__
