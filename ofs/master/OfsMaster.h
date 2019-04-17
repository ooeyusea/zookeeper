#ifndef _OFS_MASTER_H_
#define _OFS_MASTER_H_
#include "transaction.h"

namespace ofs {
	class Master : public IStateData {
	public:
		Master();
		virtual ~Master() {}

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

	class MasterFactory : public IStateDataFactory {
	public:
		MasterFactory() {}
		virtual ~MasterFactory() {}

		virtual IStateData * Create() {
			return new Master;
		}

	private:

	};
}

#endif //_OFS_MASTER_H_
