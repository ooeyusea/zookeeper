#include "PaxosImpl.h"
#include "XmlReader.h"

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
				int32_t servicePort = conf.Root()["paxos"][0]["server"][i].GetAttributeInt32("service");

				if (idx == _id) {
					_ip = ip;
					_electionPort = electionPort;
					_votePort = votePort;
					_servicePort = servicePort;
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

			if (!_election.Start(_servers.size() + 1, _ip, _electionPort, _servers)) {
				hn_error("paxos: election start failed");
				return false;
			}

			hn_info("paxos started");
		}
		catch (std::exception& e) {
			hn_error("paxos load config failed : %s", e.what());
			return false;
		}
	}

	void PaxosImpl::RegisterLogType(ITransaction * transaction) {
		_dataset->RegisterLogType(transaction);
	}

	void PaxosImpl::DoTransaction(ITransaction * transaction, const char * param, int32_t size) {
		bool success = false;
		int64_t transactionId = 0;
		{
			std::lock_guard<hn_mutex> guard(_mutex);
			while (true) {
				transactionId = _nextTransaction++;
				if (_waitTransactions.find(transactionId) == _waitTransactions.end()) {
					hn_set(&success);
					_waitTransactions[transactionId] = { transaction, hn_current, olib::GetTickCount() };
					break;
				}
			}
		}



		hn_block;

		if (!success)
			throw std::logic_error("do failed");
	}
}
