#include "follow.h"
#include "define.h"
#include "socket_helper.h"

#define FIRST_SLEEP_INTERVAL 50
#define TRY_COUNT 5

Follow::Follow(int32_t id, DataSet& dataset)
	: _id(id)
	, _dataset(dataset)
	, _service(*this) {

}

Follow::~Follow() {

}

int32_t Follow::Following(int32_t peerEpoch, const std::string& leaderIp, int32_t leaderPort, int32_t servicePort) {
	for (int32_t i = 0; i < TRY_COUNT; ++i) {
		hn_sleep FIRST_SLEEP_INTERVAL;

		_fd = hn_connect(leaderIp.c_str(), leaderPort);
		if (_fd > 0)
			break;
	}

	if (_fd < 0)
		return peerEpoch;

	hn_send(_fd, (const char*)&_id, sizeof(_id));

	try {
		RegisterToLeader(peerEpoch);

		SyncWithLeader();

		if (!_service.Start(servicePort))
			return peerEpoch;

		ProcossPropose();
	}
	catch (std::exception& e) {
		hn_error("follow exception: {}", e.what());
	}

	_service.Stop();
	hn_close(_fd);
	return peerEpoch;
}

void Follow::Propose(std::string && data, const std::function<void(bool)>& fn) {
	int64_t requestId = 0;
	{
		std::lock_guard<hn_mutex> guard(_lock);
		requestId = _nextRequestId++;
		_callbacks[requestId] = fn;
	}

	std::string buf;
	buf.resize(sizeof(paxos_def::Request));
	buf.append(data);

	paxos_def::Request& info = *(paxos_def::Request*)buf.data();
	info = { paxos_def::PX_REQUEST, requestId, (int32_t)data.size() };

	hn_send(_fd, buf.data(), buf.size());
}

void Follow::Read(std::string && data, std::string& result) {
	_dataset.Read(std::forward<std::string>(data), result);
}

void Follow::RegisterToLeader(int32_t& peerEpoch) {
	paxos_def::FollowInfo info = { paxos_def::PX_FOLLOW_INFO, peerEpoch };
	hn_send(_fd, (const char*)&info, sizeof(info));

	paxos_def::LeaderInfo leaderInfo;
	SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_LEADER_INFO, leaderInfo);

	if (peerEpoch < leaderInfo.peerEpoch) {
		peerEpoch = leaderInfo.peerEpoch;
	}
	else if (peerEpoch > leaderInfo.peerEpoch) {
		throw std::logic_error("self peer epoch is bigger than leader");
	}

	paxos_def::AckEpoch ack = { paxos_def::PX_ACK_EPOCH, _dataset.GetZxId(), peerEpoch };
	hn_send(_fd, (const char*)&ack, sizeof(ack));
}

void Follow::SyncWithLeader() {
	int8_t type;
	SocketReader().ReadType(_fd, type);

	switch (type) {
	case paxos_def::PX_TRUNC_DATASET: {
			paxos_def::Trunc trunc = { paxos_def::PX_TRUNC_DATASET };
			SocketReader().ReadRest<int8_t>(_fd, trunc);

			_dataset.Trunc(trunc.zxId);
		}
		break;
	case paxos_def::PX_SNAP_DATASET: {
			paxos_def::Snap snap = { paxos_def::PX_SNAP_DATASET };
			SocketReader().ReadRest<int8_t>(_fd, snap);
			std::string data = SocketReader().ReadBlock(_fd, snap.size);

			_dataset.Snap(snap.zxId, std::move(data));
		}
		break;
	case paxos_def::PX_DIFF_DATASET: break;
	}

	bool synced = false;
	while (!synced) {
		SocketReader().ReadType(_fd, type);

		switch (type) {
		case paxos_def::PX_UPDATED: 
			synced = true;
			break;
		case paxos_def::PX_NEWLEADER: {
				int8_t ack = paxos_def::PX_ACK;
				hn_send(_fd, (const char *)&ack, sizeof(ack));
			}
			break;
		case paxos_def::PX_PROPOSE: {
				paxos_def::Propose propose = { paxos_def::PX_PROPOSE };
				SocketReader().ReadRest<int8_t>(_fd, propose);
				std::string data = SocketReader().ReadBlock(_fd, propose.size);

				_dataset.Propose(propose.zxId, std::move(data));
			}
			break;
		case paxos_def::PX_COMMIT: {
				paxos_def::Commit commit = { paxos_def::PX_COMMIT };
				SocketReader().ReadRest<int8_t>(_fd, commit);

				_dataset.Commit(commit.zxId);
			}
			break;
		}
	}
}

void Follow::ProcossPropose() {
	while (true) {
		int8_t type;
		SocketReader().ReadType(_fd, type);

		switch (type) {
		case paxos_def::PX_PROPOSE: {
				paxos_def::Propose propose = { paxos_def::PX_PROPOSE };
				SocketReader().ReadRest<int8_t>(_fd, propose);
				std::string data = SocketReader().ReadBlock(_fd, propose.size);

				if (_dataset.Propose(propose.zxId, std::move(data))) {
					paxos_def::ProposeAck ack = { paxos_def::PX_ACK_PROPOSE, propose.zxId };
					hn_send(_fd, (const char*)&ack, sizeof(ack));
				}
				else {
					hn_error("propose {} failed", propose.zxId);
					throw std::logic_error("propose data failed");
				}
			}
			break;
		case paxos_def::PX_COMMIT: {
				paxos_def::Commit commit = { paxos_def::PX_COMMIT };
				SocketReader().ReadRest<int8_t>(_fd, commit);

				_dataset.Commit(commit.zxId);
			}
			break;
		case paxos_def::PX_PING: {
				type = paxos_def::PX_PONG;
				hn_send(_fd, (const char *)&type, sizeof(type));
			}
			break;
		case paxos_def::PX_REQUEST_ACK: {
				paxos_def::RequestAck ack = { paxos_def::PX_REQUEST_ACK };
				SocketReader().ReadRest<int8_t>(_fd, ack);

				std::lock_guard<hn_mutex> guard(_lock);
				auto itr = _callbacks.find(ack.requestId);
				if (itr != _callbacks.end()) {
					itr->second(ack.success);
					_callbacks.erase(ack.requestId);
				}
			}
			break;
		}
	}
}
