#include "Node.h"
#include "user/User.h"
#include "api/OfsMaster.pb.h"

namespace ofs {
	bool Node::CheckAuthority(User * user, bool read) {
		if (user->IsSupper())
			return true;

		if (user->GetName() == _owner)
			return read ? _authority & api::AuthorityType::AT_OWNER_READ : _authority & api::AuthorityType::AT_OWNER_WRITE;
		else if (user->GetGroup() == _ownerGroup)
			return read ? _authority & api::AuthorityType::AT_GROUP_READ : _authority & api::AuthorityType::AT_GROUP_WRITE;
		else
			return read ? _authority & api::AuthorityType::AT_OTHER_READ : _authority & api::AuthorityType::AT_OTHER_WRITE;

		return true;
	}
}
