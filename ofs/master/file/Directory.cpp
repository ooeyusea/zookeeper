#include "Directory.h"
#include "time_helper.h"
#include "File.h"
#include "api/OfsMaster.pb.h"
#include "user/User.h"
#include "FileSystem.h"

namespace ofs {
	Directory::~Directory() {
		for (auto itr = _children.begin(); itr != _children.end(); ++itr)
			delete itr->second;
		_children.clear();
	}

	int32_t Directory::CreateNode(User * user, const char * path, const char * name, int16_t authority, bool dir) {
		return QueryNode(user, path, [this, authority, dir, name](User * user, Node * node) -> int32_t {
			if (!node->IsDir())
				return api::master::ErrorCode::EC_IS_NOT_DIRECTORY;

			return (static_cast<Directory*>(node))->CreateNode(user, name, authority, dir);
		});
	}

	int32_t Directory::Remove(User * user, const char * path) {
		return QueryNode(user, path, [this](User * user, Node * node) -> int32_t {
			if (!node->CheckAuthority(user, true))
				return api::master::ErrorCode::EC_PERMISSION_DENY;

			if (node->IsRoot())
				return api::master::ErrorCode::EC_PERMISSION_DENY;

			node->MarkDelete();
			return api::master::ErrorCode::EC_NONE;
		});
	}

	std::vector<Node*> Directory::List(User * user, const char * path) {
		std::vector<Node*> ret;
		QueryNode(user, path, [this, &ret](User * user, Node * node) -> int32_t {
			if (!node->IsDir()) {
				ret.emplace_back(node);
			}
			else {
				Directory * dir = static_cast<Directory *>(node);
				ret = dir->List();
			}
			return api::master::ErrorCode::EC_NONE;
		});
		return ret;
	}

	int32_t Directory::QueryNode(User * user, const char * path, const std::function<int32_t(User * user, Node * node)>& fn) {
		if (*path == '/')
			++path;

		std::string subDir;
		const char * next = nullptr;
		std::tie(subDir, next) = FetchSubPath(path);

		if (subDir.empty())
			return fn(user, this);

		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
		auto itr = _children.find(subDir);
		if (itr == _children.end() || itr->second->IsDelete())
			return api::master::ErrorCode::EC_FILE_NOT_EIXST;

		if (next && !itr->second->IsDir())
			return api::master::ErrorCode::EC_FILE_NOT_EIXST;

		
		return !next ? fn(user, itr->second) : (static_cast<Directory*>(itr->second))->QueryNode(user, next, fn);
	}

	void Directory::BuildAllFile() {
		for (auto itr = _children.begin(); itr != _children.end(); ++itr) {
			if (itr->second->IsDir())
				static_cast<Directory*>(itr->second)->BuildAllFile();
			else
				FileSystem::Instance().AddFile(static_cast<File*>(itr->second));
		}
	}

	std::vector<Node*> Directory::List() {
		hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);

		std::vector<Node*> ret;
		for (auto itr = _children.begin(); itr != _children.end(); ++itr) {
			if (!itr->second->IsDelete())
				ret.emplace_back(itr->second);
		}
		return ret;
	}

	int32_t Directory::CreateNode(User * user, const char * name, int16_t authority, bool dir) {
		std::lock_guard<hn_shared_mutex> guard(_mutex);

		if (!CheckAuthority(user, false))
			return api::master::ErrorCode::EC_PERMISSION_DENY;

		auto itr = _children.find(name);
		if (itr != _children.end()) {
			if (itr->second->IsDelete()) {
				if ((dir && !itr->second->IsDir()) || (!dir && itr->second->IsDir()))
					return api::master::ErrorCode::EC_ALREADY_EXIST_DELETE_FILE;
				itr->second->Recover();
				return api::master::ErrorCode::EC_NONE;
			}

			return api::master::ErrorCode::EC_FILE_EIXST;
		}

		Node * node = nullptr;
		if (dir)
			node = new Directory;
		else {
			File * file = new File;
			FileSystem::Instance().AddFile(file);

			node = file;
		}

		node->SetName(name);
		node->SetOwner(user->GetName());
		node->SetOwnerGroup(user->GetGroup());
		node->SetAuthority(authority);
		node->SetParent(this);
		node->SetCreateTime(olib::GetTimeStamp());

		_children[node->GetName()] = node;
		return api::master::ErrorCode::EC_NONE;
	}

	void Directory::StartGC(int64_t now) {
		{
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);
			for (auto itr = _children.begin(); itr != _children.end(); ++itr) {
				if (itr->second->IsDir())
					(static_cast<Directory*>(itr->second))->StartGC(now);
			}
		}

		std::lock_guard<hn_shared_mutex> lock(_mutex);
		_canNotCleanUp = false;
		auto itr2 = _children.begin();
		while (itr2 != _children.end()) {
			auto itr = itr2;
			++itr2;

			if (!itr->second->IsDir()) {
				if (static_cast<File*>(itr->second)->IsUsed()) {
					_canNotCleanUp = true;
					continue;
				}
			}

			if (itr->second->IsDelete() && now > itr->second->GetDeleteTime() + FileSystem::Instance().GetDeleteExpireTime()) {
				if (!itr->second->IsDir() || static_cast<Directory*>(itr->second)->CanCleanUp()) {
					delete itr->second;
					_children.erase(itr);
				}
			}
		}
	}
} 
