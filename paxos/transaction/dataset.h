#ifndef __DATASET_H__
#define __DATASET_H__
#include "hnet.h"
#include "util.h"
#include <string>
#include "paxos.h"

#define SINGLE_LOG_FILE_COUNT 100

namespace paxos {
#pragma pack(push, 1)
	struct LogDataFileLogDesc {
		int64_t zxId;
		int32_t offset;
		int32_t size;
	};

	struct LogDataFileHeader {
		int64_t start;
		int64_t count;
		LogDataFileLogDesc logs[SINGLE_LOG_FILE_COUNT];
	};
#pragma pack(pop)

	class DataSet {
		struct Bill {
			int64_t zxId;
			std::string data;
		};

	public:
		DataSet(IStateData * stateData);
		~DataSet();

		bool Load(std::string path);

		bool Propose(int64_t zxId, std::string data);
		void Commit(int64_t zxId, ITransaction* transaction = nullptr);

		void GetSnap(std::string& data);
		void Snap(int64_t zxId, std::string data);
		void Trunc(int64_t zxId);

		void Flush();

		inline void Query(int64_t start, int64_t end, const std::function<bool(int64_t zxId, const std::string&)>& f) {
			for (auto& bill : _commitLog) {
				if (bill.zxId >= start && bill.zxId <= end) {
					if (!f(bill.zxId, bill.data))
						break;
				}
			}
		}

		inline void QueryUnCommit(const std::function<bool(int64_t zxId, const std::string&)>& f) {
			for (auto & bill : _uncommit) {
				if (!f(bill.zxId, bill.data))
					break;
			}
		}

		inline void RegisterLogType(ITransaction * transaction) {
			_transactions[transaction->GetType()] = transaction;
		}

		inline hn_shared_lock<hn_shared_mutex> LockRead() {
			return hn_shared_lock<hn_shared_mutex>(_mutex);
		}

		inline int64_t GetMinZxId() const { return _minZxId; }
		inline int64_t GetZxId() const { return _zxId; }

	private:
		bool LoadLogData(int64_t fileId);
		void TruncFile(int64_t fileId, int64_t start);
		bool SaveLogData(Bill& bill);

		void Apply(const Bill& bill, ITransaction* transaction = nullptr);
		void Rollback(const Bill& bill);

	private:
		hn_shared_mutex _mutex;

		std::string _path;
		int64_t _minZxId = 0;
		int64_t _zxId = 0;

		std::list<Bill> _commitLog;
		std::list<Bill> _uncommit;
		int32_t _snaperCount = 0;

		IStateData * _stateData = nullptr;

		FILE * _lastLogFile = nullptr;
		LogDataFileHeader _logFileheader;

		std::unordered_map<int32_t, ITransaction*> _transactions;
	};
}
#define ZXID(epoch, id) ((((int64_t)(epoch)) << 32) | ((int64_t)(id)))
#define EPOCH_FROM_ZXID(zxid) (((zxid) >> 32) & 0xFFFFFFFF)

#endif //__DATASET_H__
