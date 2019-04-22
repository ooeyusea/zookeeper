#include "Directory.h"
#include "time_helper.h"
#include "File.h"
#include "api/OfsMaster.pb.h"
#include "user/User.h"

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
		return FindNode(user, path, [authority, dir, this](User * user, const char * path) -> int32_t {
			if (!CheckAuthority(user, false))
				return api::ErrorCode::EC_PERMISSION_DENY;

			std::unique_lock<hn_shared_mutex> guard(_mutex);

			auto itr = _children.find(path);
			if (itr != _children.end()) {
				if (itr->second->IsDelete()) {
					itr->second->Recover();
					return api::ErrorCode::EC_NONE;
				}

				return api::ErrorCode::EC_FILE_EIXST;
			}

			Node * node = nullptr;
			if (dir)
				node = new Directory;
			else
				node = new File;

			node->SetName(path);
			node->SetOwner(user->GetName());
			node->SetOwnerGroup(user->GetGroup());
			node->SetAuthority(authority);
			node->SetParent(this);
			node->SetCreateTime(olib::GetTimeStamp());

			_children[node->GetName()] = node;
			return api::ErrorCode::EC_NONE;
		});
	}

	int32_t Directory::Remove(User * user, const char * path) {
		return FindNode(user, path, [this](User * user, const char * path) -> int32_t {
			std::unique_lock<hn_shared_mutex> guard(_mutex);

			auto itr = _children.find(path);
			if (itr == _children.end())
				return api::ErrorCode::EC_FILE_NOT_EIXST;

			if (itr->second->CheckAuthority(user, true))
				return api::ErrorCode::EC_PERMISSION_DENY;

			itr->second->MarkDelete();
			return api::ErrorCode::EC_NONE;
		});
	}

	std::vector<Node*> Directory::List(User * user, const char * path) {
		std::vector<Node*> ret;
		FindNode(user, path, [this, &ret](User * user, const char * path) -> int32_t {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);

			for (auto itr = _children.begin(); itr != _children.end(); ++itr) {
				if (!itr->second->IsDelete())
					ret.emplace_back(itr->second);
			}
			return api::ErrorCode::EC_NONE;
		});
		return ret;
	}

	Node* Directory::QueryNode(User * user, const char * path) {
		Node* ret = nullptr;
		FindNode(user, path, [this, &ret](User * user, const char * path) -> int32_t {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);

			auto itr = _children.find(path);
			if (itr != _children.end() && !itr->second->IsDelete())
				ret = itr->second;

			return api::ErrorCode::EC_NONE;
		});
		return ret;
	}


}
