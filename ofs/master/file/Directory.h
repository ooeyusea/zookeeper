#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__
#include "hnet.h"
#include "Node.h"
#include "File.h"

namespace ofs {
	class Directory : public Node {
	public:
		Directory() : Node(true) {}

		~Directory() {}

		virtual void DoNotWantToObject() {}
		
		template <typename Stream>
		inline void Archive(hyper_net::IArchiver<Stream>& ar) {
			Node::Archive(ar);

			int32_t count = 0;
			ar & count;
			if (ar.Fail())
				return;

			for (int32_t i = 0; i < count; ++i) {
				bool dir = false;
				ar & dir;

				if (ar.Fail())
					return;

				Node * node = nullptr;
				if (dir) {
					Directory * dir = new Directory;
					ar & *dir;

					node = dir;
				}
				else {
					File * file = new File;
					ar & *file;

					node = file;
				}

				node->SetParent(this);
				_children[node->GetName()] = node;
			}
		}

		template <typename Stream>
		inline void Archive(hyper_net::OArchiver<Stream>& ar) {
			hn_shared_lock_guard<hn_shared_mutex> guard(_mutex);

			Node::Archive(ar);
			ar & (int32_t)_children.size();
			for (auto itr = _children.begin(); itr != _children.end(); ++itr) {
				if (itr->second->IsDir()) {
					bool dir = true;
					ar & dir;
					ar & *static_cast<Directory*>(itr->second);
				}
				else {
					bool dir = false;
					ar & dir;
					ar & *static_cast<File*>(itr->second);
				}
			}
		}

		int32_t CreateNode(User * user, const char * path, const char * name, int16_t authority, bool dir);
		int32_t Remove(User * user, const char * path);
		std::vector<Node*> List(User * user, const char * path);
		int32_t QueryNode(User * user, const char * path, const std::function<int32_t(User * user, Node * node)>& fn);
		void BuildAllFile();

		inline bool Empty() const { return _children.empty(); }

		inline std::tuple<std::string, const char*> FetchSubPath(const char * path) {
			const char * slash = strchr(path, '/');
			if (slash)
				return std::make_tuple(std::string(path, slash - path), slash + 1);
			
			return std::make_tuple(std::string(path), nullptr);
		}

	private:
		std::vector<Node*> List();
		int32_t CreateNode(User * user, const char * name, int16_t authority, bool dir);

	private:
		std::unordered_map<std::string, Node*> _children;
	};
}

#endif //__DIRECTORY_H__
