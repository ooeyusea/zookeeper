#ifndef _TEST_STATE_DATA_H_
#define _TEST_STATE_DATA_H_
#include "transaction.h"

namespace test {
	class TestStateData : public IStateData {
	public:
		TestStateData();
		virtual ~TestStateData() {}

		virtual void Release() { delete this; }

		virtual bool LoadFromFile(const std::string& path);
		virtual bool SaveToFile(const std::string& path);

		virtual bool PreCheck(const std::string& data);
		virtual void Apply(const std::string& data);
		virtual void Rollback(const std::string& data);

		virtual void BuildFromData(const std::string& data);
		virtual void GetData(std::string& data);

		virtual std::string Read(const std::string& data);

	private:
	};

	class TestStateDataFactory : public IStateDataFactory {
	public:
		TestStateDataFactory() {}
		virtual ~TestStateDataFactory() {}

		virtual IStateData * Create() {
			return new TestStateData;
		}

	private:

	};
}

#endif //_TEST_STATE_DATA_H_