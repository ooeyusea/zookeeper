#ifndef __DATASET_H__
#define __DATASET_H__
#include "hnet.h"
#include "util.h"
#include <string>

class Transaction;
class DataSet {
public:
	DataSet();
	~DataSet();

	bool Load(const char * path);

	void Propose(int64_t zxId, std::string data);
	void Commit(int64_t zxId);

	void GetSnap(std::string& data);
	void Snap(std::string data);
	void Trunc(int64_t zxId);

	void Flush();

	void Query(int64_t start, int64_t end);

	void LockRead();
	void LockWrite();

	inline int64_t GetMinZxId() const { return _minZxId; }
	inline int64_t GetZxId() const { return _zxId; }

private:
	int64_t _minZxId;
	int64_t _zxId;
};

#endif //__DATASET_H__
