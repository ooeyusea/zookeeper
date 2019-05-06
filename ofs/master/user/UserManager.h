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

		struct UserSaveCommand {
			std::string name;
			std::string group;
			std::string password;
			bool supper;
		};

	public:
		UserManager() {}
		~UserManager() {}

		bool Start(const olib::IXmlObject& root);

		std::string Login(const std::string& name, const std::string& password);
		bool Add(const std::string& name, const std::string& group, const std::string& password);
		bool Remove(const std::string& name);

		User * Acquire(const std::string& token);
		void Release(User * user);

		void CreateRootUser();
		void ClearExpire();
		void DoSave();

		inline void SaveUser(const std::string& name, const std::string& group, const std::string& password, bool supper) {
			_saveQueue << UserSaveCommand{ name, group, password, supper };
		}

	private:
		std::string Generate();

	private:
		std::unordered_map<std::string, User> _users;
		std::unordered_map<std::string, Auth> _auths;

		bool _terminate = false;
		int64_t _expire;
		int64_t _expireCheckInterval;

		hn_channel<UserSaveCommand, 100> _saveQueue;
	};
}

#endif //__OFS_MASTER_H__
