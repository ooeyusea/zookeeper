#include "UserManager.h"
#include "time_helper.h"
#include "file_system.h"

namespace ofs {
	bool UserManager::Start(const olib::IXmlObject& root) {
		const char * path = root["user"][0]["path"][0].GetAttributeString("val");
		_expire = root["user"][0]["token"][0].GetAttributeInt64("duration") * SECOND;

		bool hasOne = false;
		olib::FileFinder().Search("*.user", [this, &hasOne](const fs::path& file) {
			olib::XmlReader conf;
			if (!conf.LoadXml(file.string().c_str())) {
				hn_error("load user {} failed", file.filename().string());
				return;
			}

			User user;
			user.SetName(conf.Root()["name"][0].GetAttributeString("val"));
			user.SetGroup(conf.Root()["group"][0].GetAttributeString("val"));
			user.SetPassword(conf.Root()["password"][0].GetAttributeString("val"));
			if (conf.Root().IsExist("super"))
				user.SetSuper(conf.Root()["super"][0].GetAttributeBoolean("val"));

			_users[user.GetName()] = user;

			hasOne = true;
		});

		if (!hasOne)
			CreateRootUser();

		hn_fork [this]{
			ClearExpire();
		};

		hn_fork[this]{
			DoSave();
		};

		return true;
	}

	std::string UserManager::Login(const std::string& name, const std::string& password) {
		auto itr = _users.find(name);
		if (itr != _users.end() && itr->second.Check(password)) {
			itr->second.AddRef();

			std::string token = Generate();
			_auths[token] = { &(itr->second), olib::GetTickCount() + _expire };

			return token;
		}
		else
			return "";
	}

	bool UserManager::Add(const std::string& name, const std::string& group, const std::string& password) {
		auto itr = _users.find(name);
		if (itr == _users.end()) {
			User user;
			user.SetName(name);
			user.SetGroup(group);
			user.SetPassword(password);

			_users[user.GetName()] = user;
			return true;
		}
		return false;
	}

	bool UserManager::Remove(const std::string& name) {
		auto itr = _users.find(name);
		if (itr != _users.end() && !itr->second.IsUsed()) {
			_users.erase(itr);
			return true;
		}
		return false;
	}

	User * UserManager::Acquire(const std::string& token) {
		auto itr = _auths.find(token);
		if (itr != _auths.end()) {
			itr->second.user->AddRef();
			return itr->second.user;
		}
		return nullptr;
	}

	void UserManager::Release(User * user) {
		user->DecRef();
	}

	void UserManager::CreateRootUser() {
		User& root = _users["root"];

		root.SetSuper(true);
		root.SetName("root");
		root.SetGroup("root");

		SaveUser("root", "root", "", true);
	}

	void UserManager::ClearExpire() {
		while (!_terminate) {
			hn_sleep _expireCheckInterval;

			int64_t tick = olib::GetTickCount();

			auto itr2 = _auths.begin();
			while (itr2 != _auths.end()) {
				auto itr = itr2;
				++itr2;

				if (tick > itr->second.expire) {
					itr->second.user->DecRef();
					_auths.erase(itr);
				}
			}
		}
	}

	void UserManager::DoSave() {
		while (!_terminate) {
			UserSaveCommand command;
			_saveQueue >> command;

			//to do
		}
	}

	std::string UserManager::Generate() {
		static uint32_t seed = 11231234123;
		seed = seed * 12978312;

		std::string token;
		token.reserve(32);

		uint32_t key = seed;
		int32_t count = 32;
		while (key > 0 && count-- > 0) {
			char c = (char)(key % 26);
			key /= 26;

			token.push_back('a' + c);
		}

		return token;
	}
}
