#ifndef __USERMANAGER_H__
#define __USERMANAGER_H__
#include "hnet.h"
#include "singleton.h"
#include "User.h"
#include "XmlReader.h"

namespace ofs {
	class UserManager : public olib::Singleton<UserManager> {
		struct Auth {
			User* user;
			int64_t expire;
		};
	public:
		UserManager() {}
		~UserManager() {}

		bool Start(const olib::IXmlObject& root);

		std::string Login(const std::string& name, const std::string& password);

		User * Acquire(const std::string& token);
		void Release(User * user);

		void ClearExpire();

	private:
		std::string Generate();

	private:
		hn_mutex _mutex;
		std::unordered_map<std::string, User> _users;
		std::unordered_map<std::string, Auth> _auths;

		bool _terminate = false;
		int64_t _expire;
		int64_t _expireCheckInterval;
	};
}

#endif //__OFS_MASTER_H__
