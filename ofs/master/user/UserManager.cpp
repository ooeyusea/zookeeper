#include "UserManager.h"
#include "time_helper.h"

namespace ofs {
	bool UserManager::Start(const olib::IXmlObject& root) {

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
		}
		else
			return "";

		return token;
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
