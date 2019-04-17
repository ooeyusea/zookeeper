#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "hnet.h"
#include "fsm/fsm.h"

namespace yarn {
	enum ResourceStateType {
		RST_NEW = 0,
		RST_LOCALIZING,
		RST_LOCALIZED,
		RST_LOCALIZATION_FAILED,
		RST_CLEANING,
		RST_CLEANING_WAIT_RELOCALIZE,
		RST_CLEANUP,
	};

	class Resource : public fsm::StateMachine<Resource> {
	public:
		Resource() : fsm::StateMachine<Resource>(ResourceStateType::RST_NEW) {}
		~Resource() {}

		virtual void InitilizeTranform();

		inline const std::string& GetRemote() const { return _remotePath; }
		inline const std::string& GetLocal() const { return _localPath; }

		inline void AddLoadCallback(const std::function<void(bool)>& fn) { _loadCallback.push_back(fn); }
		inline void Localize(bool success) {
			for (auto& fn : _loadCallback)
				fn(success);
		}
		inline void Localized() { Localize(true); }
		inline void LocalizeFailed() { Localize(false); }

		void Download();
		void Cleanup();

	private:
		std::string _remotePath;
		std::string _localPath;

		std::list<std::function<void(bool)>> _loadCallback;
	};
}

#endif //__RESOURCE_H__
