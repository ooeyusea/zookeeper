#include "lead.h"
#include "util.h"
#include "socket_helper.h"

#define HEARTBET 200
#define HALF_BROKEN_CHECK_INTERVAL 1000
#define CHECK_BROKEN_INTERVAL 100

namespace paxos {
	class FollowHandler {
	public:
		FollowHandler(Lead & lead, DataSet& dataset, int32_t fd) : _lead(lead), _dataset(dataset), _fd(fd) {}
		~FollowHandler() {}

		void Start();

	private:
		void SyncFollower(int64_t lastPeerZxId);
		void Snap(int64_t zxId);
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
			olib::SocketReader().ReadType(_fd, _id);
			_lead.Register(_id, _fd);

			paxos_def::FollowInfo follow;
			olib::SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_FOLLOW_INFO, follow);

			int32_t peerEpoch = _lead.GetPeerEpoch(_id, follow.peerEpoch);
			paxos_def::LeaderInfo leader{ paxos_def::PX_LEADER_INFO, peerEpoch };
			hn_send(_fd, (const char*)&leader, sizeof(leader));

			paxos_def::AckEpoch ackEpoch;
			olib::SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_ACK_EPOCH, ackEpoch);

			SyncFollower(ackEpoch.lastZxId);

			int8_t type = paxos_def::PX_NEWLEADER;
			hn_send(_fd, (const char*)&type, sizeof(type));

			olib::SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_ACK, type);

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

		_terminate = true;
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
			_dataset.Query(_dataset.GetMinZxId(), _dataset.GetZxId(), [this, &first, &prevZxId, lastPeerZxId](int64_t zxId, const std::string& data) {
				if (zxId < lastPeerZxId) {
					prevZxId = zxId;
					return true;
				}

				if (first) {
					if (zxId == lastPeerZxId) {
						int8_t type = paxos_def::PX_DIFF_DATASET;
						hn_send(_fd, (const char*)&type, sizeof(type));
					}
					else if (zxId > lastPeerZxId) {
						if (EPOCH_FROM_ZXID(zxId) != EPOCH_FROM_ZXID(lastPeerZxId)) {
							Snap(_dataset.GetZxId());
							return false;
						}
						else {
							paxos_def::Trunc trunc = { paxos_def::PX_TRUNC_DATASET, prevZxId };
							hn_send(_fd, (const char*)&trunc, sizeof(trunc));
						}
					}
				}
				else {
					std::string buf;
					buf.reserve(data.size() + sizeof(paxos_def::Propose));
					buf.resize(sizeof(paxos_def::Propose));

					paxos_def::Propose& propose = *(paxos_def::Propose*)buf.data();
					propose = { paxos_def::PX_PROPOSE, zxId, (int32_t)data.size() };
					buf.append(data.data(), data.size());

					hn_send(_fd, buf.data(), buf.size());

					paxos_def::Commit commit = { paxos_def::PX_COMMIT, zxId };
					hn_send(_fd, (const char*)&commit, sizeof(commit));
				}
				return true;
			});
		}
		else if (lastPeerZxId < _dataset.GetMinZxId()) {
			Snap(_dataset.GetZxId());
		}

		_dataset.QueryUnCommit([this](int64_t zxId, const std::string& data) {
			std::string buf;
			buf.reserve(data.size() + sizeof(paxos_def::Propose));
			buf.resize(sizeof(paxos_def::Propose));

			paxos_def::Propose& propose = *(paxos_def::Propose*)buf.data();
			propose = { paxos_def::PX_PROPOSE, zxId, (int32_t)data.size() };
			buf.append(data.data(), data.size());

			hn_send(_fd, buf.data(), buf.size());
			return true;
		});

		guard.unlock();

		_lead.StartForwarding(_id, _fd);
	}

	void FollowHandler::Snap(int64_t zxId) {
		std::string data;
		data.resize(sizeof(paxos_def::Snap));

		paxos_def::Snap& snap = *(paxos_def::Snap*)data.data();
		snap.type = paxos_def::PX_SNAP_DATASET;
		snap.zxId = zxId;

		_dataset.GetSnap(data);
		snap.size = (int32_t)(data.size() - sizeof(paxos_def::Snap));

		hn_send(_fd, data.data(), data.size());
	}

	void FollowHandler::Process() {
		TickSendPing();

		while (!_terminate) {
			int8_t type;
			olib::SocketReader().ReadType(_fd, type);

			switch (type) {
			case paxos_def::PX_PONG: break;
			case paxos_def::PX_REQUEST: {
				paxos_def::Request info = { paxos_def::PX_REQUEST };
				olib::SocketReader().ReadRest<int8_t>(_fd, info);

				std::string data = olib::SocketReader().ReadBlock(_fd, info.size);
				_lead.Propose(_id, info.requestId, std::move(data));
			}
										break;
			case paxos_def::PX_ACK_PROPOSE: {
				paxos_def::ProposeAck info = { paxos_def::PX_ACK_PROPOSE };
				olib::SocketReader().ReadRest<int8_t>(_fd, info);

				_lead.AckPropose(_id, info.zxId);
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

	Lead::Lead(int32_t id, DataSet& dataset, int32_t serverCount)
		: _id(id)
		, _serverCount(serverCount)
		, _dataset(dataset)
		, _service(*this) {
	}

	Lead::~Lead() {

	}

	int32_t Lead::Leading(int32_t peerEpoch, int32_t votePort, int32_t servicePort) {
		if (!StartListenFollow(votePort))
			return peerEpoch;

		try {
			peerEpoch = GetPeerEpoch(_id, peerEpoch);

			WaitForNewLeader(_id);

			if (!_service.Start(servicePort))
				return peerEpoch;

			Process();

			_service.Stop();
			_dataset.Flush();
		}
		catch (std::exception& e) {
			hn_error("leading exception {}", e.what());
		}

		_service.Stop();
		ShutdownListen();

		return peerEpoch;
	}

	void Lead::Propose(std::string && data, const std::function<void(bool)>& fn) {
		int64_t requestId = 0;
		{
			std::lock_guard<hn_mutex> guard(_lock);
			requestId = _nextRequestId++;
			_callbacks[requestId] = fn;
		}

		Propose(_id, requestId, std::forward<std::string>(data));
	}

	void Lead::Read(std::string && data, std::string& result) {
		_dataset.Read(std::forward<std::string>(data), result);
	}

	void Lead::Propose(int32_t id, int64_t requestId, std::string && data) {
		std::unique_lock<hn_mutex> guard(_lock);
		int64_t zxId = ZXID(_peerEpoch, _nextId);
		++_nextId;
		_uncommit[zxId] = { id, requestId, zxId, data };
		_uncommit[zxId].vote.resize(_forwarding.size());
		_uncommit[zxId].vote[id - 1] = true;

		if (_uncommit.size() == 1)
			LaunchNextDraft(guard);
	}

	void Lead::AckPropose(int32_t id, int64_t zxId) {
		std::unique_lock<hn_mutex> guard(_lock);
		auto itr = _uncommit.begin();
		if (itr != _uncommit.end() && itr->first == zxId) {
			itr->second.vote[id - 1] = true;

			int32_t count = std::count(itr->second.vote.begin(), itr->second.vote.end(), true);
			if (count >= (int32_t)itr->second.vote.size() / 2 + 1) {
				_dataset.Commit(zxId);
				int64_t requestId = itr->second.requestId;
				int32_t id = itr->second.id;
				_uncommit.erase(zxId);

				paxos_def::Commit commit{ paxos_def::PX_COMMIT, zxId };
				for (auto fd : _forwarding) {
					if (fd > 0) {
						hn_send(fd, (const char*)&commit, sizeof(commit));
					}
				}

				if (id == _id) {
					std::lock_guard<hn_mutex> cbGuard(_cbLock);
					auto itr = _callbacks.find(requestId);
					if (itr != _callbacks.end()) {
						itr->second(true);
						_callbacks.erase(requestId);
					}
				}
				else {
					paxos_def::RequestAck ack = { paxos_def::PX_REQUEST_ACK, requestId, true };

					int32_t fd = _forwarding[id - 1];
					if (fd > 0) {
						hn_send(fd, (const char*)&ack, sizeof(ack));
					}
				}

				if (!_uncommit.empty())
					LaunchNextDraft(guard);
			}
		}
	}


	int32_t Lead::GetPeerEpoch(int32_t id, int32_t peerEpoch) {
		if (!_waitForPeerEpoch)
			return _peerEpoch;

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
		_peerEpoch = peerEpoch + 1;
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

	void Lead::Process() {
		int64_t tick = zookeeper::GetTickCount();
		while (true) {
			int64_t now = zookeeper::GetTickCount();

			int32_t count = (int32_t)_forwarding.size() - std::count(_forwarding.begin(), _forwarding.end(), 0);
			if (count > (int32_t)_forwarding.size() / 2 + 1)
				tick = now;
			else if (now >= tick + HALF_BROKEN_CHECK_INTERVAL)
				break;

			hn_sleep CHECK_BROKEN_INTERVAL;
		}
	}

	void Lead::LaunchNextDraft(std::unique_lock<hn_mutex>& guard) {
		while (!_uncommit.empty()) {
			int64_t zxId = _uncommit.begin()->first;
			std::string& data = _uncommit.begin()->second.data;
			if (_dataset.CheckOrDrop(data) && _dataset.Propose(zxId, data)) {
				std::string buf;
				buf.reserve(data.size() + sizeof(paxos_def::Propose));
				buf.resize(sizeof(paxos_def::Propose));

				paxos_def::Propose& propose = *(paxos_def::Propose*)data.data();
				propose = { paxos_def::PX_PROPOSE, zxId, (int32_t)data.size() };
				data.append(data.data(), data.size());

				guard.unlock();

				for (auto fd : _forwarding) {
					if (fd > 0) {
						hn_send(fd, buf.data(), buf.size());
					}
				}

				break;
			}
			else {
				int64_t requestId = _uncommit.begin()->second.requestId;
				int32_t id = _uncommit.begin()->second.id;
				_uncommit.erase(zxId);

				guard.unlock();

				if (id != _id) {
					paxos_def::RequestAck ack = { paxos_def::PX_REQUEST_ACK, requestId, false };

					int32_t fd = _forwarding[id - 1];
					if (fd > 0) {
						hn_send(fd, (const char*)&ack, sizeof(ack));
					}
				}
				else {
					std::lock_guard<hn_mutex> cbGuard(_cbLock);
					auto itr = _callbacks.find(requestId);
					if (itr != _callbacks.end()) {
						itr->second(false);
						_callbacks.erase(requestId);
					}
				}
			}
		}
	}

	bool Lead::StartListenFollow(int32_t votePort) {
		_followers.resize(_serverCount, 0);
		_forwarding.resize(_serverCount, 0);

		_recvPeerEpoch.resize(_serverCount, 0);
		_recvNewLeader.resize(_serverCount, false);

		_fd = hn_listen("0.0.0.0", votePort);
		if (_fd < 0)
			return false;

		++_count;
		hn_fork[this]{
			while (true) {
				int32_t remoteFd = hn_accept(_fd);
				if (remoteFd < 0)
					break;

				++_count;
				hn_fork[this, remoteFd] {
					FollowHandler handler(*this, _dataset, remoteFd);
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

		while (_count > 0) {
			hn_sleep 100;
		}
	}
}
