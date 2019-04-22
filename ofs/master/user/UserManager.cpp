#include "UserManager.h"
#include "time_helper.h"
#include "file_system.h"

namespace ofs {
	bool UserManager::Start(const olib::IXmlObject& root) {
		const char * path = root["user"][0].GetAttributeString("path");
		olib::FileFinder().Search("*.user", [this](const fs::path& file) {
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
		});

		hn_fork [this]{
			ClearExpire();
		};
		return true;
	}

	std::string UserManager::Login(const std::string& name, const std::string& password) {
		std::unique_lock<hn_mutex> _mutex;
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
		std::unique_lock<hn_mutex> _mutex;
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
		std::unique_lock<hn_mutex> _mutex;
		auto itr = _users.find(name);
		if (itr != _users.end() && !itr->second.IsUsed()) {
			_users.erase(itr);
			return true;
		}
		return false;
	}

	User * UserManager::Acquire(const std::string& token) {
		std::unique_lock<hn_mutex> _mutex;
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

	void UserManager::ClearExpire() {
		while (!_terminate) {
			hn_sleep _expireCheckInterval;

			int64_t tick = olib::GetTickCount();

			std::unique_lock<hn_mutex> _mutex;
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

	std::string UserManager::Generate() {
		static int32_t seed = 11231234123;
		seed = seed * 12978312;

		std::string token;
		token.reserve(32);

		int32_t key = seed;
		int32_t count = 32;
		while (key > 0 && count-- > 0) {
			char c = (char)(key % 26);
			key /= 26;

			token.push_back('a' + c);
		}
	}
}
