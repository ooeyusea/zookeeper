#include "PaxosImpl.h"
#include "XmlReader.h"
#include "lead.h"
#include "follow.h"

namespace paxos {
	PaxosImpl::PaxosImpl(IStateData * data) {
		_dataset = new DataSet(data);
	}

	PaxosImpl::~PaxosImpl() {
		delete _dataset;
	}

	bool PaxosImpl::Start(const std::string& path) {
		try {
			olib::XmlReader conf;
			if (conf.LoadXml(path.c_str()))
				return false;

			_id = conf.Root()["paxos"][0].GetAttributeInt32("id");
			if (_id == 0) {
				hn_error("paxos: invalid id");
				return false;
			}

			_clientPort = conf.Root()["paxos"][0].GetAttributeInt32("clientport");

			const auto& servers = conf.Root()["paxos"][0]["server"];
			for (int32_t i = 0; i < servers.Count(); ++i) {

				int32_t idx = conf.Root()["paxos"][0]["server"][i].GetAttributeInt32("id");
				std::string ip = conf.Root()["paxos"][0]["server"][i].GetAttributeString("ip");
				int32_t electionPort = conf.Root()["paxos"][0]["server"][i].GetAttributeInt32("election");
				int32_t votePort = conf.Root()["paxos"][0]["server"][i].GetAttributeInt32("vote");

				if (idx == _id) {
					_ip = ip;
					_electionPort = electionPort;
					_votePort = votePort;
				}
				else
					_servers.push_back({ idx, ip, electionPort, votePort });
			}

			if (_servers.size() % 2 != 0) {
				hn_error("paxos: server count must old");
				return false;
			}

			std::string dataPath = conf.Root()["paxos"][0]["data"][0].GetAttributeString("path");
			if (!_dataset->Load(dataPath)) {
				return false;
			}

			_peerEpoch = EPOCH_FROM_ZXID(_dataset->GetZxId());

			if (!_election.Start((int32_t)_servers.size() + 1, _ip, _electionPort, _servers)) {
				hn_error("paxos: election start failed");
				return false;
			}

			hn_fork[this]{
				while (true) {
					switch (_state) {
					case Election::LOOKING: Elect(); break;
					case Election::FOLLOWING: Following(); break;
					case Election::LEADING: Leading(); break;
					}
				}
			};

			hn_info("paxos started");
		}
		catch (std::exception& e) {
			hn_error("paxos load config failed : %s", e.what());
			return false;
		}

		return true;
	}

	void PaxosImpl::RegisterLogType(ITransaction * transaction) {
		_dataset->RegisterLogType(transaction);
	}

	void PaxosImpl::DoTransaction(ITransaction * transaction, const char * param, int32_t size) {
		bool success = false;
		hn_set(&success);

		if (_executor && _executor->IsActive())
			_executor->Propose(std::string(param, size), transaction);
		else
			success = false;

		if (!success)
			throw std::logic_error("do failed");
	}

	void PaxosImpl::Elect() {
		Vote vote = _election.LookForLeader(_id, _dataset->GetZxId(), (int32_t)_servers.size() + 1);

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

	void PaxosImpl::Leading() {
		hn_info("I'm leader\n");

		paxos::Lead lead(_id, *_dataset, (int32_t)_servers.size() + 1);
		_peerEpoch = lead.Leading(_peerEpoch, _votePort, this);
		lead.ClearRequest();
	}

	void PaxosImpl::Following() {
		hn_info("I'm follower\n");

		paxos::Follow follow(_id, *_dataset);
		_peerEpoch = follow.Following(_peerEpoch, _leader->ip, _leader->votePort, this);
		follow.ClearRequest();
	}
}
