#include "Directory.h"
#include "time_helper.h"
#include "File.h"
#include "api/OfsMaster.pb.h"

namespace ofs {
	int32_t Directory::FindNode(User * user, const char * path, const std::function<int32_t(User * user, const char * path)>& fn) {
		const char * slash = strchr(path, '/');
		if (slash != nullptr) {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);

			std::string name(path, slash - path);
			auto itr = _children.find(name);
			if (itr == _children.end())
				return api::ErrorCode::EC_FILE_NOT_EIXST;

			if (!itr->second->IsDir())
				return api::ErrorCode::EC_FILE_NOT_EIXST;

			return static_cast<Directory*>(itr->second)->FindNode(user, slash + 1, fn);
		}
		else
			return fn(user, path);
	}

	int32_t Directory::CreateNode(User * user, const char * path, int16_t authority, bool dir) {
		FindNode(user, path, [authority, dir, this](User * user, const char * path) -> int32_t {
			if (!CheckAuthority(user, false))
				return api::ErrorCode::EC_PERMISSION_DENY;

			std::unique_lock<hn_shared_mutex> guard(_mutex);

			auto itr = _children.find(path);
			if (itr != _children.end())
				return api::ErrorCode::EC_FILE_EIXST;

			Node * node = nullptr;
			if (dir)
				node = new Directory;
			else
				node = new File;

			node->SetName(path);
			node->SetOwner(user->GetName());
			node->GetOwnerGroup(user->GetGroup());
			node->SetAuthority(authority);
			node->SetParent(this);
			node->SetCreateTime(olib::GetTimeStamp());

			_children[node->GetName()] = node;
			return 0;
		});
	}

	int32_t Directory::Remove(User * user, const char * path) {
		FindNode(user, path, [this](User * user, const char * path) -> int32_t {
			std::unique_lock<hn_shared_mutex> guard(_mutex);

			auto itr = _children.find(path);
			if (itr == _children.end())
				return api::ErrorCode::EC_FILE_NOT_EIXST;

			if (itr->second->CheckAuthority(user, true))
				return api::ErrorCode::EC_PERMISSION_DENY;

			if (itr->second->IsDir()) {
				Directory * dir = static_cast<Directory*>(itr->second);
				if (!dir->Empty())
					return api::ErrorCode::EC_DIR_NOT_EMPTY;
			}
			else {
				File * file = static_cast<File*>(itr->second);
				file->ClearChunk();
			}

			delete itr->second;
			_children.erase(itr);
			return 0;
		});
	}

	std::vector<Node*> Directory::List(User * user, const char * path) {
		std::vector<Node*> ret;
		FindNode(user, path, [this, &ret](User * user, const char * path) -> int32_t {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);

			for (auto itr = _children.begin(); itr != _children.end(); ++itr)
				ret.emplace_back(itr->second);
			return 0;
		});
		return ret;
	}

	int32_t Directory::Open(User * user, const char * path, bool write) {
		const char * slash = strchr(path, '/');
		if (slash != nullptr) {
			_mutex.lock_shared();

			std::string name(path, slash - path);
			auto itr = _children.find(name);
			if (itr == _children.end()) {
				_mutex.unlock_shared();
				return api::ErrorCode::EC_FILE_NOT_EIXST;
			}

			if (!itr->second->IsDir()) {
				_mutex.unlock_shared();
				return api::ErrorCode::EC_FILE_NOT_EIXST;
			}

			int32_t ret = static_cast<Directory*>(itr->second)->Open(user, slash + 1, write);
			if (ret < 0)
				_mutex.unlock_shared();

			return ret;
		}
		else {
			if (!CheckAuthority(user, false))
				return api::ErrorCode::EC_PERMISSION_DENY;

			_mutex.lock_shared();

			auto itr = _children.find(path);
			if (itr == _children.end()) {
				_mutex.unlock_shared();
				return api::ErrorCode::EC_FILE_NOT_EIXST;
			}

			if (itr->second->IsDir()) {
				_mutex.unlock_shared();
				return api::ErrorCode::EC_IS_DIR;
			}

			itr->second->Lock(write);
			return user->AddOpenFile(path, write);
		}
	}

	int32_t Directory::Close(User * user, int32_t fd) {
		std::string path;
		bool write;
		std::tie(path, write) = user->RemoveOpenFile(fd);

		Close(user, path.c_str(), write);
	}

	int32_t Directory::Close(User * user, const char * path, bool write) {
		const char * slash = strchr(path, '/');
		if (slash != nullptr) {
			std::string name(path, slash - path);
			auto itr = _children.find(name);
			if (itr == _children.end())
				return api::ErrorCode::EC_FILE_NOT_EIXST;

			if (!itr->second->IsDir())
				return api::ErrorCode::EC_FILE_NOT_EIXST;

			int32_t ret = static_cast<Directory*>(itr->second)->Close(user, slash + 1, write);
			if (ret == 0)
				_mutex.unlock_shared();

			return ret;
		}
		else {
			if (!CheckAuthority(user, false))
				return api::ErrorCode::EC_PERMISSION_DENY;

			auto itr = _children.find(path);
			if (itr == _children.end())
				return api::ErrorCode::EC_FILE_NOT_EIXST;

			if (itr->second->IsDir())
				return api::ErrorCode::EC_IS_DIR;
			
			
			itr->second->Unlock(write);
			_mutex.unlock_shared();
			return 0;
		}
	}
}
