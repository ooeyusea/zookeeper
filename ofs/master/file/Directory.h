#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__
#include "hnet.h"
#include "Node.h"

namespace ofs {
	struct LockContinue {
		int32_t errCode = 0;
		hn_co co = hn_current;
		User * user;
		std::string& path;
		bool read;
	};

	class Directory : public Node {
	public:
		Directory() : Node(true) {
			_size = 0;
		}

		~Directory() {}

		virtual void DoNotWantToObject() {}

		int32_t CreateNode(User * user, const char * path, int16_t authority, bool dir);
		int32_t Remove(User * user, const char * parttern);

		int32_t Lock(User * user, const char * path, bool read, LockContinue& ret);
		void UnLock(User * user, const char * path);

		void ContinueLock(LockContinue& ret);

		inline bool Empty() const { return _children.empty(); }

	private:
		std::unordered_map<std::string, Node*> _children;
	};
}

#endif //__DIRECTORY_H__
