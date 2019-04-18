#include "hnet.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include "election.h"
#include "XmlReader.h"
#include "lead.h"
#include "follow.h"
#include "dataset.h"
#include "paxos.h"

namespace paxos {
	bool Paxos::Start(IStateData * data, const std::string& path) {
		try {
			olib::XmlReader conf;
			if (conf.LoadXml(path.c_str()))
				return false;

			_id = conf.Root()["zookeeper"][0].GetAttributeInt32("id");
			if (_id == 0) {
				hn_error("zookeeper: invalid id");
				return false;
			}

			const auto& servers = conf.Root()["zookeeper"][0]["server"];
			for (int32_t i = 0; i < servers.Count(); ++i) {

				int32_t idx = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("id");
				std::string ip = conf.Root()["zookeeper"][0]["server"][i].GetAttributeString("ip");
				int32_t electionPort = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("election");
				int32_t votePort = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("vote");
				int32_t servicePort = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("service");

				if (idx == _id) {
					_ip = ip;
					_electionPort = electionPort;
					_votePort = votePort;
					_servicePort = servicePort;
				}
				else
					_servers.push_back({ idx, ip, electionPort, votePort });
			}
		}
		catch (std::exception& e) {
			hn_error("zookeeper load config failed : %s", e.what());
			return false;
		}

		if (_servers.size() % 2 != 0) {
			hn_error("zookeeper: server count must old");
			return false;
		}

#ifdef WIN32
		if (!_dataset->Load("var/lib/zookeeper")) {
#else
		if (!_dataset->Load("/var/lib/zookeeper")) {
#endif
			return false;
		}

		_peerEpoch = EPOCH_FROM_ZXID(_dataset->GetZxId());

		if (!_election.Start(_servers.size() + 1, _ip, _electionPort, _servers)) {
			hn_error("zookeeper: election start failed");
			return false;
		}

		hn_info("zookeeper started");
		return true;
	}

	void Paxos::DoTransaction(ITransaction * transaction, const char * param, int32_t size) {
	}


	Paxos::Paxos() {

	}
}

class ZooKeeper {
public:
	ZooKeeper() {}
	~ZooKeeper() {}

	bool Start();
	void Run();

	void Elect();
	void Leading();
	void Following();

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

bool ZooKeeper::Start() {
	try {
		olib::XmlReader conf;
#ifdef WIN32
		if (conf.LoadXml("etc/zookeeper/conf.xml"))
#else
		if (conf.LoadXml("/etc/zookeeper/conf.xml"))
#endif
			return false;

		_id = conf.Root()["zookeeper"][0].GetAttributeInt32("id");
		if (_id == 0) {
			hn_error("zookeeper: invalid id");
			return false;
		}

		_clientPort = conf.Root()["zookeeper"][0].GetAttributeInt32("clientport");

		const auto& servers = conf.Root()["zookeeper"][0]["server"];
		for (int32_t i = 0; i < servers.Count(); ++i) {

			int32_t idx = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("id");
			std::string ip = conf.Root()["zookeeper"][0]["server"][i].GetAttributeString("ip");
			int32_t electionPort = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("election");
			int32_t votePort = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("vote");
			int32_t servicePort = conf.Root()["zookeeper"][0]["server"][i].GetAttributeInt32("service");

			if (idx == _id) {
				_ip = ip;
				_electionPort = electionPort;
				_votePort = votePort;
				_servicePort = servicePort;
			}
			else
				_servers.push_back({ idx, ip, electionPort, votePort });
		}

		std::string path = conf.Root()["zookeeper"][0]["state_data"][0].GetAttributeString("path");
#ifdef linux
		void * handle = dlopen(path, RTLD_LAZY);
		GetFactoryFn fn = (GetFactoryFn)dlsym(handle, NAME_OF_GET_FACTORY_FN);
#endif //linux

#ifdef WIN32
		HINSTANCE hinst = ::LoadLibrary(path.c_str());
		GetFactoryFn fn = (GetFactoryFn)::GetProcAddress(hinst, NAME_OF_GET_FACTORY_FN);
#endif //WIN32

		if (!fn) {
			hn_error("get state data module failed");
			return false;
		}
	}
	catch (std::exception& e) {
		hn_error("zookeeper load config failed : %s", e.what());
		return false;
	}

	if (_servers.size() % 2 != 0) {
		hn_error("zookeeper: server count must old");
		return false;
	}

#ifdef WIN32
	if (!_dataset->Load("var/lib/zookeeper")) {
#else
	if (!_dataset->Load("/var/lib/zookeeper")) {
#endif
		return false;
	}

	_peerEpoch = EPOCH_FROM_ZXID(_dataset->GetZxId());

	if (!_election.Start(_servers.size() + 1, _ip, _electionPort, _servers)) {
		hn_error("zookeeper: election start failed");
		return false;
	}

	hn_info("zookeeper started");
	return true;
}

void ZooKeeper::Run() {
	while (true) {
		switch (_state) {
		case Election::LOOKING: Elect(); break;
		case Election::FOLLOWING: Following(); break;
		case Election::LEADING: Leading(); break;
		}
	}
}

void ZooKeeper::Elect() {
	Vote vote = _election.LookForLeader(_id, _dataset->GetZxId(), _servers.size() + 1);

	_state = (vote.voteId == _id) ? Election::LEADING : Election::FOLLOWING;
	if (_state == Election::FOLLOWING) {
		for (auto& server : _servers) {
			if (server.id == vote.idx) {
				_leader = &server;
				break;
			}
		}
	}
}

void ZooKeeper::Leading() {
	hn_info("I'm leader\n");

	Lead lead(_id, *_dataset, _servers.size() + 1);
	_peerEpoch = lead.Leading(_peerEpoch, _votePort, _servicePort);
}

void ZooKeeper::Following() {
	hn_info("I'm follower\n");

	Follow follow(_id, *_dataset);
	_peerEpoch = follow.Following(_peerEpoch, _leader->ip, _leader->votePort, _servicePort);
}

void start(int32_t argc, char ** argv) {
	ZooKeeper keeper;
	if (keeper.Start())
		keeper.Run();
}
