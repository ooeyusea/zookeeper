#include "lead.h"
#include "util.h"
#include "socket_helper.h"

#define HEARTBET 200
#define ZXID(epoch, id) (((epoch) << 32) | (id))
#define EPOCH_FROM_ZXID(zxid) (((zxid) >> 32) & 0xFFFFFFFF)

class FollowHandler {
public:
	FollowHandler(Lead & lead, DataSet& dataset, int32_t fd) : _lead(lead), _dataset(dataset), _fd(fd) {}
	~FollowHandler() {}

	void Start();

private:
	void SyncFollower(int64_t lastPeerZxId);
	void Snap();
	void Process();

	void TickSendPing();
	void WaitPingStop();

private:
	Lead & _lead;
	DataSet& _dataset;
	int32_t _fd;

	int32_t _id = 0;
	int32_t _count = 0;
	bool _terminate = false;
};

void FollowHandler::TickSendPing() {
	++_count;

	hn_fork[this]{
		while (!_terminate) {
			hn_sleep HEARTBET;

			int32_t type = paxos_def::PX_PING;
			hn_send(_fd, (const char*)&type, sizeof(type));
		}

		--_count;
	};
}

void FollowHandler::Start() {
	try {
		SocketReader().ReadType(_fd, _id);
		_lead.Register(_id, _fd);

		paxos_def::FollowInfo follow;
		SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_FOLLOW_INFO, follow);

		int32_t peerEpoch = _lead.GetPeerEpoch(_id, follow.peerEpoch);
		paxos_def::LeaderInfo leader{ paxos_def::PX_LEADER_INFO, peerEpoch };
		hn_send(_fd, (const char*)&leader, sizeof(leader));

		paxos_def::AckEpoch ackEpoch;
		SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_ACK_EPOCH, ackEpoch);

		SyncFollower(ackEpoch.lastZxId);

		int8_t type = paxos_def::PX_NEWLEADER;
		hn_send(_fd, (const char*)&type, sizeof(type));

		SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_ACK, type);

		_lead.WaitForNewLeader(_id);

		type = paxos_def::PX_UPDATED;
		hn_send(_fd, (const char*)&type, sizeof(type));

		Process();
	}
	catch (hn_channel_close_exception& e) {
		hn_info("message channel close {}", _id);
	}
	catch (std::exception & e) {
		hn_info("follower close {}", _id);
	}

	if (_id > 0)
		_lead.Unregister(_id, _fd);

	_terminate = false;
	WaitPingStop();
}

void FollowHandler::SyncFollower(int64_t lastPeerZxId) {
	auto guard = _dataset.LockRead();

	if (lastPeerZxId == _dataset.GetZxId()) {
		int8_t type = paxos_def::PX_DIFF_DATASET;
		hn_send(_fd, (const char*)&type, sizeof(type));
	}
	else if (lastPeerZxId > _dataset.GetZxId()) {
		paxos_def::Trunc trunc = { paxos_def::PX_TRUNC_DATASET, _dataset.GetZxId() };
		hn_send(_fd, (const char*)&trunc, sizeof(trunc));
	}
	else if (lastPeerZxId >= _dataset.GetMinZxId() && lastPeerZxId <= _dataset.GetZxId()) {
		bool first = true;
		int64_t prevZxId = _dataset.GetMinZxId();
		for (auto & log : _dataset.Query(_dataset.GetMinZxId(), _dataset.GetZxId())) {
			if (log.GetZxId() < lastPeerZxId) {
				prevZxId = log.GetZxId();
				continue;
			}

			if (first) {
				if (log.GetZxId() == lastPeerZxId) {
					int8_t type = paxos_def::PX_DIFF_DATASET;
					hn_send(_fd, (const char*)&type, sizeof(type));
				}
				else if (log.GetZxId() > lastPeerZxId) {
					if (EPOCH_FROM_ZXID(log.GetZxId()) != EPOCH_FROM_ZXID(lastPeerZxId)) {
						Snap();
						break;
					}
					else {
						paxos_def::Trunc trunc = { paxos_def::PX_TRUNC_DATASET, prevZxId };
						hn_send(_fd, (const char*)&trunc, sizeof(trunc));
					}
				}
			}
			else {
				std::string data;
				data.reserve(log.GetSize() + sizeof(paxos_def::Propose));
				data.resize(sizeof(paxos_def::Propose));

				paxos_def::Propose& propose = *(paxos_def::Propose*)data.data();
				propose = { paxos_def::PX_PROPOSE, log.GetZxId(), log.GetSize() };
				data.append(log.GetData(), log.GetSize());

				hn_send(_fd, data.data(), data.size());

				paxos_def::ProposeAck ackPropose = { paxos_def::PX_ACK_PROPOSE, log.GetZxId() };
				hn_send(_fd, (const char*)&ackPropose, sizeof(ackPropose));
			}
		}
	}
	else if (lastPeerZxId < _dataset.GetMinZxId()) {
		Snap();
	}

	_lead.StartForwarding(_id, _fd);
}

void FollowHandler::Snap() {
	std::string data;
	data.resize(sizeof(paxos_def::Snap));

	paxos_def::Snap& snap = *(paxos_def::Snap*)data.data();
	snap.type = paxos_def::PX_SNAP_DATASET;

	_dataset.GetSnap(data);
	snap.size = (int32_t)(data.size() - sizeof(paxos_def::Snap));

	hn_send(_fd, data.data(), data.size());
}

void FollowHandler::Process() {
	TickSendPing();

	while (!_terminate) {
		int8_t type;
		SocketReader().ReadType(_fd, type);

		switch (type) {
		case paxos_def::PX_PONG: break;
		case paxos_def::PX_REQUEST: {
				paxos_def::Request info = { paxos_def::PX_REQUEST };
				SocketReader().ReadRest<int8_t>(_fd, info);

				std::string data = SocketReader().ReadBlock(_fd, info.size);
				_lead.Propose(_id, data);
			}
			break;
		case paxos_def::PX_ACK_PROPOSE: {
				paxos_def::ProposeAck info = { paxos_def::PX_ACK_PROPOSE };
				SocketReader().ReadRest<int8_t>(_fd, info);

				_lead.AckPropose(_id, info.zxId, _dataset);
			}
			break;
		default:
			hn_warn("unexcept message");
			break;
		}
	}
}

void FollowHandler::WaitPingStop() {
	while (true) {
		hn_sleep HEARTBET;

		if (_count == 0)
			break;
	}
}

Lead::Lead() {
}

Lead::~Lead() {

}

int32_t Lead::Leading(int32_t id, int32_t peerEpoch, DataSet& dataset, int32_t votePort, int32_t serverCount) {
	if (!StartListenFollow(id, serverCount, votePort, dataset))
		return peerEpoch;

	try {
		peerEpoch = GetPeerEpoch(id, peerEpoch);

		WaitForNewLeader(id);

		ProcessRequest(id, dataset, serverCount, peerEpoch);

		dataset.Flush();
	}
	catch (hn_channel_close_exception& e) {
		hn_error("half follower close");
	}
	catch (std::exception& e) {
		hn_error("leading exception {}", e.what());
	}

	ShutdownListen();

	return peerEpoch;
}

int32_t Lead::GetPeerEpoch(int32_t id, int32_t peerEpoch) {
	if (_waitForPeerEpoch) {

	}

	_recvPeerEpoch[id - 1] = peerEpoch;

	while (true) {
		int32_t count = 0;
		for (auto epoch : _recvPeerEpoch) {
			if (epoch > 0)
				++count;
		}

		if (count >= (int32_t)_recvPeerEpoch.size() / 2 + 1) {
			hn_sleep 500;
			break;
		}
		else
			hn_sleep 100;
	}

	for (auto epoch : _recvPeerEpoch) {
		if (epoch > 0 && epoch > peerEpoch)
			peerEpoch = epoch;
	}

	_waitForPeerEpoch = false;
	return peerEpoch + 1;
}

void Lead::WaitForNewLeader(int32_t id) {
	_recvNewLeader[id - 1] = true;

	while (true) {
		int32_t count = 0;
		for (auto synced : _recvNewLeader) {
			if (synced)
				++count;
		}

		if (count >= (int32_t)_recvPeerEpoch.size() / 2 + 1)
			break;
		else {
			hn_sleep 100;
		}

		if (_terminate) {
			throw std::logic_error("c++ error");
		}
	}
}

void Lead::ProcessRequest(int32_t id, DataSet& dataset, int32_t serverCount, int32_t peerEpoch) {
	while (true) {
		Message msg;
		_messages >> msg;

		switch (*(int8_t*)msg.data.data()) {
		case paxos_def::PX_REQUEST: {
				DealRequest(msg.id, msg.data);
			}
			break;
		case paxos_def::PX_ACK_PROPOSE: {
				paxos_def::ProposeAck& ackPropose = *(paxos_def::ProposeAck*)msg.data.data();
				DealAckPropose(msg.id, ackPropose.zxId, dataset);
			}
			break;
		}
	}
}

bool Lead::StartListenFollow(int32_t id, int32_t serverCount, int32_t votePort, DataSet& dataset) {
	_followers.resize(serverCount, 0);
	_forwarding.resize(serverCount, 0);

	_recvPeerEpoch.resize(serverCount, 0);
	_recvNewLeader.resize(serverCount, false);

	_fd = hn_listen("0.0.0.0", votePort);
	if (_fd < 0)
		return false;

	++_count;
	hn_fork [this, &dataset] {
		while (true) {
			int32_t remoteFd = hn_accept(_fd);
			if (remoteFd < 0)
				break;

			++_count;
			hn_fork [this, remoteFd, &dataset] {
				FollowHandler handler(*this, dataset, remoteFd);
				handler.Start();

				hn_shutdown(remoteFd);
				--_count;
			};
		}
		--_count;
	};

	return true;
}

void Lead::ShutdownListen() {
	hn_shutdown(_fd);
	_messages.Close();

	while (_count > 0) {
		hn_sleep 100;
	}
}

void Lead::SendLeaderInfo(int32_t id, int32_t peerEpoch) {
	paxos_def::LeaderInfo info{ paxos_def::PX_LEADER_INFO, peerEpoch };
	
	int32_t fd = _followers[id - 1];
	if (fd > 0) {
		hn_send(fd, (const char*)&info, sizeof(info));
	}
}

void Lead::Diff(int32_t id, DataSet& dataset, int64_t lastZxId) {
	int32_t fd = _followers[id - 1];
	if (fd > 0) {
		int8_t type = paxos_def::PX_DIFF_DATASET;
		hn_send(fd, (const char*)&type, sizeof(type));

		for (auto& log : dataset.StartWith(lastZxId)) {
			std::string data;
			data.reserve(log.GetSize() + sizeof(paxos_def::Propose));
			data.resize(sizeof(paxos_def::Propose));

			paxos_def::Propose& propose = *(paxos_def::Propose*)data.data();
			propose = { paxos_def::PX_PROPOSE, log.GetZxId(), log.GetSize() };
			data.append(log.GetData(), log.GetSize());

			hn_send(fd, data.data(), data.size());

			paxos_def::ProposeAck ackPropose = { paxos_def::PX_ACK_PROPOSE, log.GetZxId() };
			hn_send(fd, (const char*)&ackPropose, sizeof(ackPropose));
		}

		type = paxos_def::PX_NEWLEADER;
		hn_send(fd, (const char*)&type, sizeof(type));
	}
}

void Lead::Trunc(int32_t id, DataSet& dataset, int64_t lastZxId) {
	int32_t fd = _followers[id - 1];
	if (fd > 0) {
		paxos_def::Trunc trunc = { paxos_def::PX_TRUNC_DATASET, dataset.GetZxId() };
		hn_send(fd, (const char*)&trunc, sizeof(trunc));

		int8_t type = paxos_def::PX_NEWLEADER;
		hn_send(fd, (const char*)&type, sizeof(type));
	}
}

void Lead::Snap(int32_t id, DataSet& dataset) {

}

void Lead::Updated(int32_t id, DataSet& dataset) {

}

void Lead::Propose(int32_t id, const std::string& data) {

}

void Lead::AckPropose(int32_t id, int64_t zxId, DataSet& dataset) {

}
