#ifndef __SERVICE_H__
#define __SERVICE_H__
#include "hnet.h"
#include "paxos.h"

namespace paxos {
	class ServiceExecutor {
		struct TransactionCall {
			ITransaction * transaction;
			hn_co co;
		};
	public:
		ServiceExecutor() {}
		virtual ~ServiceExecutor() {}

		virtual void Propose(std::string && data, ITransaction *) = 0;

		bool IsActive() const { return _active; }

		inline void ClearRequest() {
			std::lock_guard<hn_mutex> guard(_transactionLock);
			for (auto itr = _callbacks.begin(); itr != _callbacks.end(); ++itr) {
				hn_co co = itr->second.co;

				bool& success = *static_cast<bool*>(hn_get(co));
				success = false;
				hn_resume(co);
			}
			_callbacks.clear();
		}
	
	protected:
		inline int64_t PushRequest(ITransaction * transaction) {
			std::lock_guard<hn_mutex> guard(_transactionLock);
			int64_t requestId = _nextRequestId++;
			_callbacks[requestId] = { transaction, hn_current };
			return requestId;
		}

		inline std::tuple<ITransaction*, hn_co> PopRequest(int64_t requestId) {
			ITransaction * transaction = nullptr;
			hn_co co = nullptr;

			{
				std::lock_guard<hn_mutex> guard(_transactionLock);
				auto itr = _callbacks.find(requestId);
				if (itr != _callbacks.end()) {
					transaction = itr->second.transaction;
					co = itr->second.co;

					_callbacks.erase(itr);
				}
			}
			return std::make_tuple(transaction, co);
		}

	protected:
		bool _active = false;

	private:
		hn_mutex _transactionLock;
		std::unordered_map<int64_t, TransactionCall> _callbacks;
		int64_t _nextRequestId = 1;
	};
}

#endif //__SERVICE_H__
