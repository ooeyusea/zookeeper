#include "dataset.h"
#include "file_system.h"

#define SNAP_COUNT 100000

namespace paxos {
	DataSet::DataSet(IStateData * stateData)
		: _mutex(false)
		, _stateData(stateData) {
	}

	DataSet::~DataSet() {
		_stateData->Release();
	}

	bool DataSet::Load(std::string path) {
		if (!path.empty() && *path.rbegin() != '/')
			path += "/";
		_path = path;

		std::vector<int64_t> snapers;
		olib::FileFinder().Search(path + "*.snap", [this, &snapers](const fs::path filePath) {
			snapers.push_back(atoll(filePath.filename().stem().string().c_str()));
		});

		std::sort(snapers.begin(), snapers.end(), std::greater<int64_t>());
		auto itr = snapers.begin();
		while (itr != snapers.end()) {
			char file[1024];
			snprintf(file, sizeof(file), "%s%lld.snap", path.c_str(), *itr);

			if (_stateData->LoadFromFile(file)) {
				_minZxId = *itr;
				break;
			}
			else {
				fs::remove(file);
			}
		}

		_zxId = _minZxId;

		std::vector<int64_t> logdatas;
		olib::FileFinder().Search(path + "*.logdata", [this, &logdatas](const fs::path filePath) {
			int64_t logId = atoll(filePath.filename().stem().string().c_str());
			if (logId + SINGLE_LOG_FILE_COUNT > _zxId)
				logdatas.push_back(logId);
		});

		std::sort(logdatas.begin(), logdatas.end());
		for (auto log : logdatas) {
			if (!LoadLogData(log)) {
				for (auto log2 : logdatas) {
					if (log2 >= log) {
						char file[1024];
						snprintf(file, sizeof(file), "%s%lld.logdata", path.c_str(), log2);
						fs::remove(file);
					}
				}
				break;
			}
		}

		_snaperCount = SNAP_COUNT + rand() % SNAP_COUNT;
		return true;
	}

	bool DataSet::Propose(int64_t zxId, std::string data) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);

		_uncommit.push_back({ zxId, std::move(data) });

		if (!SaveLogData(_uncommit.back())) {
			_uncommit.pop_back();
			return false;
		}

		return true;
	}

	void DataSet::Commit(int64_t zxId, ITransaction* transaction) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);

		if (_uncommit.empty() || _uncommit.begin()->zxId != zxId) {
			throw std::logic_error("invalid zxId");
		}
		else {
			_commitLog.push_back(std::move(_uncommit.front()));
			_uncommit.pop_front();

			Apply(_commitLog.back(), transaction);
			
			_zxId = zxId;

			if (_commitLog.size() >= _snaperCount) {
				char file[1024];
				snprintf(file, 1023, "%s%lld.snap", _path.c_str(), _zxId);

				if (!_stateData->SaveToFile(file)) {
					hn_error("save snap {} data file failed", _zxId);
					throw std::logic_error("save snap file failed");
				}

				while (_commitLog.size() > _snaperCount / 4) {
					_commitLog.pop_front();
				}

				_snaperCount = SNAP_COUNT + rand() % SNAP_COUNT;
			}
		}
	}

	void DataSet::GetSnap(std::string& data) {
		_stateData->GetData(data);
	}

	void DataSet::Snap(int64_t zxId, std::string data) {
		{
			std::lock_guard<hn_shared_mutex> guard(_mutex);

			_minZxId = zxId;
			_zxId = zxId;

			_stateData->BuildFromData(data);

			while (!_commitLog.empty() && _commitLog.back().zxId > zxId) {
				auto& log = _commitLog.back();
				_commitLog.pop_back();
			}
		}

		olib::FileFinder().Search(_path + "*.snap", [this](const fs::path filePath) {
			int64_t logId = atoll(filePath.filename().stem().string().c_str());
			if (logId > _zxId) {
				if (!fs::remove(filePath)) {
					hn_error("remove snap {} failed", logId);

					throw std::logic_error("remove snap failed");
				}
			}
		});

		if (_lastLogFile) {
			fclose(_lastLogFile);
			_lastLogFile = nullptr;
		}

		memset(&_logFileheader, 0, sizeof(_logFileheader));

		int64_t biggest = 0;
		olib::FileFinder().Search(_path + "*.logdata", [this, &biggest](const fs::path filePath) {
			int64_t logId = atoll(filePath.filename().stem().string().c_str());
			if (logId > _zxId) {
				if (!fs::remove(filePath)) {
					hn_error("remove logdata {} failed", logId);

					throw std::logic_error("remove file failed");
				}
			}
			else if (logId > biggest)
				biggest = logId;
		});

		if (biggest != 0)
			TruncFile(biggest, zxId);
	}

	void DataSet::Trunc(int64_t zxId) {
		{
			std::lock_guard<hn_shared_mutex> guard(_mutex);
			while (!_commitLog.empty() && _commitLog.back().zxId > zxId) {
				Rollback(_commitLog.back());

				_commitLog.pop_back();
			}
		}

		_zxId = zxId;

		if (_lastLogFile) {
			fclose(_lastLogFile);
			_lastLogFile = nullptr;
		}

		memset(&_logFileheader, 0, sizeof(_logFileheader));

		int64_t biggest = 0;
		olib::FileFinder().Search(_path + "*.logdata", [this, &biggest](const fs::path filePath) {
			int64_t logId = atoll(filePath.filename().stem().string().c_str());
			if (logId > _zxId) {
				char file[1024];
				snprintf(file, sizeof(file), "%s%lld.logdata", _path.c_str(), logId);

				if (!fs::remove(file)) {
					hn_error("remove logdata {} failed", logId);

					throw std::logic_error("remove file failed");
				}
			}
			else if (logId > biggest)
				biggest = logId;
		});

		if (biggest != 0)
			TruncFile(biggest, zxId);
	}

	void DataSet::Flush() {
		std::lock_guard<hn_shared_mutex> guard(_mutex);

		while (!_uncommit.empty()) {
			_commitLog.push_back(std::move(_uncommit.front()));
			_uncommit.pop_front();

			Apply(_commitLog.back());

			_zxId = _commitLog.back().zxId;
		}
	}

	bool DataSet::LoadLogData(int64_t fileId) {
		char file[1024];
		snprintf(file, sizeof(file), "%s%lld.logdata", _path.c_str(), fileId);

		if (_lastLogFile) {
			fclose(_lastLogFile);
			_lastLogFile = nullptr;
		}

		_lastLogFile = fopen(file, "rb+");
		if (!_lastLogFile) {
			hn_error("load log data file {} failed", fileId);
			return false;
		}

		if (fread(&_logFileheader, sizeof(_logFileheader), 1, _lastLogFile) < 1) {
			hn_error("invalid log data file {}", fileId);
			return false;
		}

		for (int32_t i = 0; i < _logFileheader.count; ++i) {
			if (_logFileheader.logs[i].zxId == 0)
				break;

			if (_logFileheader.logs[i].zxId > _zxId) {
				_zxId = _logFileheader.logs[i].zxId;

				if (fseek(_lastLogFile, _logFileheader.logs[i].offset, SEEK_SET) < 0) {
					hn_error("log data file {} corrupted", fileId);
					return false;
				}

#ifdef WIN32
				char * fileData = (char*)alloca(_logFileheader.logs[i].size);
#else
				char data[header.logs[i].size];
#endif
				if (fread(fileData, _logFileheader.logs[i].size, 1, _lastLogFile) < 1) {
					hn_error("log data file {} corrupted", fileId);
					return false;
				}

				_commitLog.push_back({ _zxId, std::string(fileData, _logFileheader.logs[i].size) });

				Apply(_commitLog.back());
				hn_debug("load log data file {}", fileId);
			}
		}

		return true;
	}

	void DataSet::TruncFile(int64_t fileId, int64_t start) {
		char file[1024];
		snprintf(file, sizeof(file), "%s%lld.logdata", _path.c_str(), fileId);

		_lastLogFile = fopen(file, "rb+");
		if (!_lastLogFile) {
			hn_error("load log data file {} failed", fileId);

			throw std::logic_error("trunc log data file failed");
		}

		if (fread(&_logFileheader, sizeof(_logFileheader), 1, _lastLogFile) < 1) {
			hn_error("invalid log data file {}", fileId);
			fclose(_lastLogFile);
			_lastLogFile = nullptr;

			throw std::logic_error("trunc log data file failed");
		}

		int32_t size = sizeof(_logFileheader);
		int32_t count = 0;
		for (int32_t i = 0; i < _logFileheader.count; ++i) {
			if (_logFileheader.logs[i].zxId == 0)
				break;

			if (_logFileheader.logs[i].zxId > start) {
				_logFileheader.logs[i].zxId = 0;
			}
			else
				++count;
		}
		_logFileheader.count = count;

		fseek(_lastLogFile, 0, SEEK_SET);
		if (fwrite(&_logFileheader, sizeof(_logFileheader), 1, _lastLogFile) < 1) {
			hn_error("update log data file {} header failed", fileId);
			throw std::logic_error("trunc log data file failed");
		}

		hn_debug("trunc log data file {} to {}", fileId, start);
	}

	bool DataSet::SaveLogData(Bill& bill) {
		if (_lastLogFile) {
			if (_logFileheader.count >= SINGLE_LOG_FILE_COUNT) {
				_logFileheader.count = 0;

				fclose(_lastLogFile);
				_lastLogFile = nullptr;
			}
		}

		if (!_lastLogFile) {
			_logFileheader.start = bill.zxId;
			_logFileheader.count = 0;

			char file[1024];
			snprintf(file, sizeof(file), "%s%lld.logdata", _path.c_str(), bill.zxId);

			_lastLogFile = fopen(file, "rb+");
			if (!_lastLogFile)
				return false;
		}

		int32_t idx = _logFileheader.count++;
		_logFileheader.logs[idx].zxId = bill.zxId;
		_logFileheader.logs[idx].size = (int32_t)bill.data.size();
		if (_logFileheader.count == 0)
			_logFileheader.logs[idx].offset = sizeof(_logFileheader);
		else
			_logFileheader.logs[idx].offset = _logFileheader.logs[idx - 1].offset + _logFileheader.logs[idx - 1].size;

		fseek(_lastLogFile, _logFileheader.logs[idx].offset, SEEK_SET);
		if (fwrite(bill.data.data(), bill.data.size(), 1, _lastLogFile) < 1) {
			hn_error("log data file {} append log {} failed", _logFileheader.start, bill.zxId);
			return false;
		}

		fseek(_lastLogFile, 0, SEEK_SET);
		if (fwrite(&_logFileheader, sizeof(_logFileheader), 1, _lastLogFile) < 1) {
			hn_error("update log data file {} header failed", _logFileheader.start);
			return false;
		}

		return true;
	}

	void DataSet::Apply(const Bill& bill, ITransaction * transaction) {
		hn_istream stream(bill.data.c_str(), (int32_t)bill.data.size());
		hn_iachiver ar(stream, 0);

		int32_t type;
		ar >> type;

		if (!stream.bad() && (transaction || _transactions[type])) {
			if (transaction)
				transaction->Apply(_stateData, ar);
			else
				_transactions[type]->Apply(_stateData, ar);
		}
		else {
			hn_error("invalid log : {}", bill.zxId);
			throw std::logic_error("invalid log");
		}
	}

	void DataSet::Rollback(const Bill& bill) {
		hn_istream stream(bill.data.c_str(), (int32_t)bill.data.size());
		hn_iachiver ar(stream, 0);

		int32_t type;
		ar >> type;

		if (!stream.bad() && _transactions[type])
			_transactions[type]->Rollback(_stateData, ar);
		else {
			hn_error("invalid log : {}", bill.zxId);
			throw std::logic_error("invalid log");
		}
	}
}
