#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__
#include "hnet.h"
#include "Node.h"

namespace ofs {
	class Directory : public Node {
	public:
		Directory() : Node(true) {}

		~Directory() {}

		virtual void DoNotWantToObject() {}

		int32_t FindNode(User * user, const char * path, const std::function<int32_t(User * user, const char * path)>& fn);

		int32_t CreateNode(User * user, const char * path, int16_t authority, bool dir);
		int32_t Remove(User * user, const char * parttern);

		std::vector<Node*> List(User * user, const char * parttern);
		int32_t Open(User * user, const char * path, bool write);
		int32_t Close(User * user, int32_t fd);

		inline bool Empty() const { return _children.empty(); }

	private:
		int32_t Close(User * user, const char * path, bool write);

	private:
		std::unordered_map<std::string, Node*> _children;
	};
}

#endif //__DIRECTORY_H__
