#ifndef __NODE_H__
#define __NODE_H__
#include "hnet.h"
#include "time_helper.h"
#include "XmlReader.h"

namespace ofs {
	class User;
	class Node {
	public:
		Node(bool dir) : _dir(dir), _mutex(true) {}
		virtual ~Node() {}

		virtual void DoNotWantToObject() = 0;

		template <typename AR>
		void Archive(AR& ar) {
			ar & _name;
			ar & _owner;
			ar & _ownerGroup;
			ar & _authority;
			ar & _createTime;
			ar & _updateTime;
			ar & _delete;
			ar & _deleteTick;
		}

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

		inline int64_t GetCreateTime() const { return _createTime; }
		inline void SetCreateTime(int64_t val) { _createTime = val; _updateTime = val; }

		inline int64_t GetUpdateTime() const { return _updateTime; }
		inline void SetUpdateTime(int64_t val) { _updateTime = val; }

		inline uint32_t GetSize() const { return _size; }

		inline void SetParent(Node * val) { _parent = val; }
		inline Node * GetParent() const { return _parent; }

		inline void MarkDelete() { _delete = true; _deleteTick = olib::GetTimeStamp(); }
		inline void Recover() { _delete = false; }
		inline bool IsDelete() const { return _delete; }

		bool CheckAuthority(User * user, bool read);

	protected:
		mutable hn_shared_mutex _mutex;
		std::string _name;
		std::string _owner;
		std::string _ownerGroup;
		int16_t _authority;
		bool _dir;
		int64_t _createTime;
		int64_t _updateTime;
		uint32_t _size = 0;

		bool _delete;
		int64_t _deleteTick;

		Node * _parent = nullptr;
	};
}

#endif //__NODE_H__
