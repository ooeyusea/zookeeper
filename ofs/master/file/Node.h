#ifndef __NODE_H__
#define __NODE_H__
#include "hnet.h"

namespace ofs {
	class User;
	class Node {
	public:
		Node(bool dir) : _dir(dir) {}
		virtual ~Node() {}

		virtual void DoNotWantToObject() = 0;

		inline const std::string& GetName() const { return _name; }
		inline void SetName(const std::string& val) { _name = val; }
		inline void SetName(std::string&& val) { _name = val; }
		 
		inline const std::string& GetOwner() const { return _owner; }
		inline void SetOwner(const std::string& val) { _owner = val; }
		inline void SetOwner(std::string&& val) { _owner = val; }

		inline const std::string& GetOwnerGroup() const { return _ownerGroup; }
		inline void SetOwnerGroup(const std::string& val) { _ownerGroup = val; }
		inline void SetOwnerGroup(std::string&& val) { _ownerGroup = val; }

		inline int16_t GetAuthority() const { return _authority; }
		inline void SetAuthority(int16_t val) { _authority = val; }

		inline bool IsDir() const { return _dir; }

		inline int64_t GetUpdateTime() const { return _updateTime; }
		inline void SetUpdateTime(int64_t val) { _updateTime = val; }

		inline int32_t GetSize() const { return _size; }

		inline void SetParent(Node * val) { _parent = val; }
		inline Node * GetParent() const { return _parent; }

		bool CheckLock(User * user, bool read);

	protected:
		std::string _name;
		std::string _owner;
		std::string _ownerGroup;
		int16_t _authority;
		bool _dir;
		int64_t _updateTime;
		int32_t _size;

		Node * _parent = nullptr;
	};
}

#endif //__NODE_H__

