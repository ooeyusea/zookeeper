#include "follow.h"
#include "define.h"
#include "socket_helper.h"
#include "PaxosImpl.h"

#define FIRST_SLEEP_INTERVAL 50
#define TRY_COUNT 5

namespace paxos {
	Follow::Follow(int32_t id, DataSet& dataset)
		: _id(id)
		, _dataset(dataset) {

	}

	Follow::~Follow() {

	}

	int32_t Follow::Following(int32_t peerEpoch, const std::string& leaderIp, int32_t leaderPort, PaxosImpl * impl) {
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

			_active = true;
			impl->SetExecutor(this);

			ProcossPropose();

			_active = false;
			impl->SetExecutor(nullptr);
		}
		catch (std::exception& e) {
			hn_error("follow exception: {}", e.what());
		}

		hn_close(_fd);
		return peerEpoch;
	}

	void Follow::Propose(std::string && data, ITransaction * transaction) {
		int64_t requestId = PushRequest(transaction);

		std::string buf;
		buf.resize(sizeof(paxos_def::Request));
		buf.append(data);

		paxos_def::Request& info = *(paxos_def::Request*)buf.data();
		info = { paxos_def::PX_REQUEST, requestId, (int32_t)data.size() };

		hn_send(_fd, buf.data(), (int32_t)buf.size());

		hn_block;
	}

	void Follow::RegisterToLeader(int32_t& peerEpoch) {
		paxos_def::FollowInfo info = { paxos_def::PX_FOLLOW_INFO, peerEpoch };
		hn_send(_fd, (const char*)&info, sizeof(info));

		paxos_def::LeaderInfo leaderInfo;
		olib::SocketReader().ReadMessage<int8_t>(_fd, paxos_def::PX_LEADER_INFO, leaderInfo);

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
		olib::SocketReader().ReadType(_fd, type);

		switch (type) {
		case paxos_def::PX_TRUNC_DATASET: {
				paxos_def::Trunc trunc = { paxos_def::PX_TRUNC_DATASET };
				olib::SocketReader().ReadRest<int8_t>(_fd, trunc);

				_dataset.Trunc(trunc.zxId);
			}
			break;
		case paxos_def::PX_SNAP_DATASET: {
				paxos_def::Snap snap = { paxos_def::PX_SNAP_DATASET };
				olib::SocketReader().ReadRest<int8_t>(_fd, snap);
				std::string data = olib::SocketReader().ReadBlock(_fd, snap.size);

				_dataset.Snap(snap.zxId, std::move(data));
			}
			break;
		case paxos_def::PX_DIFF_DATASET: break;
		}

		bool synced = false;
		while (!synced) {
			olib::SocketReader().ReadType(_fd, type);

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
					olib::SocketReader().ReadRest<int8_t>(_fd, propose);
					std::string data = olib::SocketReader().ReadBlock(_fd, propose.size);

					_dataset.Propose(propose.zxId, std::move(data));
				}
				break;
			case paxos_def::PX_COMMIT: {
					paxos_def::Commit commit = { paxos_def::PX_COMMIT };
					olib::SocketReader().ReadRest<int8_t>(_fd, commit);

					_dataset.Commit(commit.zxId);
				}
				break;
			}
		}
	}

	void Follow::ProcossPropose() {
		while (true) {
			int8_t type;
			olib::SocketReader().ReadType(_fd, type);

			switch (type) {
			case paxos_def::PX_PROPOSE: {
					paxos_def::Propose propose = { paxos_def::PX_PROPOSE };
					olib::SocketReader().ReadRest<int8_t>(_fd, propose);
					std::string data = olib::SocketReader().ReadBlock(_fd, propose.size);

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
					olib::SocketReader().ReadRest<int8_t>(_fd, commit);

					ITransaction * transaction = nullptr;
					hn_co co = nullptr;
					if (commit.fromId == _id)
						std::tie(transaction, co) = PopRequest(commit.requestId);

					_dataset.Commit(commit.zxId, transaction);

					if (co) {
						hn_resume(co);
					}
				}
				break;
			case paxos_def::PX_PING: {
					type = paxos_def::PX_PONG;
					hn_send(_fd, (const char *)&type, sizeof(type));
				}
				break;
			case paxos_def::PX_REQUEST_FAIL: {
					paxos_def::RequestFail ack = { paxos_def::PX_REQUEST_FAIL };
					olib::SocketReader().ReadRest<int8_t>(_fd, ack);

					ITransaction * transaction = nullptr;
					hn_co co = nullptr;
					std::tie(transaction, co) = PopRequest(ack.requestId);
					if (co) {
						bool& success = *static_cast<bool*>(hn_get(co));
						success = false;

						hn_resume(co);
					}
				}
				break;
			}
		}
	}
}