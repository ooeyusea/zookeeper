#include "Directory.h"

namespace ofs {
	int32_t Directory::CreateNode(User * user, const char * path, int16_t authority, bool dir) {
		const char * slash = strchr(path, '/');
		if (slash != nullptr) {
			if (!CheckLock(user, true))
				return -4;

			std::string name(path, slash - path);
			auto itr = _children.find(name);
			if (itr == _children.end())
				return -1;

			if (!itr->second->IsDir())
				return  -2;

			return static_cast<Directory*>(itr->second)->CreateNode(user, slash + 1, authority, dir);
		}
		else {
			if (!CheckLock(user, false))
				return -4;

			if (!CheckAuthority(user, false))
				return -3;

			auto itr = _children.find(path);
			if (itr == _children.end())
				return -5;

			Node * node = nullptr;
			if (dir)
				node = new Directory;

			node->SetName(path);
			node->SetOwner(user->GetName());
			node->GetOwnerGroup(user->GetGroup());
			node->SetAuthority(authority);
			return 0;
		}
	}

	int32_t Directory::Remove(User * user, const char * path) {
		const char * slash = strchr(path, '/');
		if (slash != nullptr) {
			if (!CheckLock(user, true))
				return -4;

			std::string name(path, slash - path);
			auto itr = _children.find(name);
			if (itr == _children.end())
				return -1;

			if (!itr->second->IsDir())
				return  -2;

			return static_cast<Directory*>(itr->second)->Remove(user, slash + 1);
		}
		else {
			if (!CheckLock(user, false))
				return -4;

			if (!CheckAuthority(user, false))
				return -3;

			auto itr = _children.find(path);
			if (itr == _children.end())
				return -1;

			if (itr->second->IsDir()) {
				Directory * dir = static_cast<Directory*>(itr->second);
				if (!dir->Empty())
					return -6;
			}
			else {

			}

			delete itr->second;
			_children.erase(itr);
			return 0;
		}
	}

	int32_t Directory::Lock(User * user, const char * path, bool read, LockContinue& ret) {
		if (strcmp(path, "") == 0) {
			if (!CheckAuthority(user, read))
				return -3;

			if (!DoLock(user, read, path, read, ret))
				return -7;

			return 0;
		}

		if (!DoLock(user, true, path, read, ret))
			return -7;

		const char * slash = strchr(path, '/');
		if (slash != nullptr) {
			std::string name(path, slash - path);
			auto itr = _children.find(name);
			if (itr == _children.end())
				return -1;

			if (!itr->second->IsDir())
				return -2;

			return static_cast<Directory*>(itr->second)->Lock(user, slash + 1, read, ret);
		}
		else {
			auto itr = _children.find(path);
			if (itr == _children.end())
				return -1;

			if (itr->second->IsDir())
				return static_cast<Directory*>(itr->second)->Lock(user, "", read, ret);
			else {

			}

			return 0;
		}
	}

	void Directory::ContinueLock(LockContinue& ret) {
		int32_t result = Lock(ret.user, ret.path.c_str(), ret.read, ret);
		if (result != -7) {
			ret.errCode = result;
			hn_resume(ret.co);
		}
	}

	void Directory::UnLock(User * user, const char * path) {
		if (!HasLock(user))
			return;

		if (strcmp(path, "") != 0) {
			const char * slash = strchr(path, '/');
			if (slash != nullptr) {
				std::string name(path, slash - path);
				auto itr = _children.find(name);
				if (itr == _children.end())
					return;

				if (!itr->second->IsDir())
					return;

				static_cast<Directory*>(itr->second)->UnLock(user, slash + 1);
			}
			else {

				auto itr = _children.find(path);
				if (itr == _children.end())
					return;

				if (itr->second->IsDir())
					return static_cast<Directory*>(itr->second)->UnLock(user, "/");
				else {

				}
			}
		}

		DoUnlock(user);
	}
}
