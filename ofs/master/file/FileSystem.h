#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__
#include "paxos.h"

namespace ofs {
	class FileSystem : public paxos::IStateData {
	public:
		FileSystem() {}
		~FileSystem() {}

		virtual void Release() {}

		virtual bool LoadFromFile(const std::string& path);
		virtual bool SaveToFile(const std::string& path);

		virtual void BuildFromData(const std::string& data);
		virtual void GetData(std::string& data);

	private:

	};
}

#endif //__FILE_SYSTEM_H__
