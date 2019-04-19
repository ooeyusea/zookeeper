#include "hnet.h"
#include "paxos.h"
#include "PaxosImpl.h"

namespace paxos {
	Paxos::Paxos(IStateData * data) {
		_impl = new PaxosImpl(data);
	}
}
