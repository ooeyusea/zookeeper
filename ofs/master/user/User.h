#ifndef __USER_H__
#define __USER_H__
#include "hnet.h"

namespace ofs {
	class User {
	public:
		User() {}
		~User() {}

		inline const std::string& GetName() const { return _name; }
		inline void SetName(const std::string& val) { _name = val; }
		inline void SetName(std::string&& val) { _name = val; }

		inline const std::string& GetGroup() const { return _group; }
		inline void SetGroup(const std::string& val) { _group = val; }
		inline void SetGroup(std::string&& val) { _group = val; }

		inline const std::string& GetPassword() const { return _password; }
		inline void SetPassword(const std::string& val) { _password = val; }
		inline void SetPassword(std::string&& val) { _password = val; }

		inline bool IsSupper() const { return _super; }
		inline void SetSuper(bool val) { _super = val; }

		inline bool Check(const std::string& password) { return _password == password; }

		inline void AddRef() { ++_count; }
		inline void DecRef() { --_count; }
		inline bool IsUsed() const { return _count > 0; }

	private:
		std::string _name;
		std::string _group;
		std::string _password;
		bool _super = false;

		int32_t _count = 0;
	};
}

#endif //__USER_H__
