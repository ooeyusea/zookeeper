#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "hnet.h"
#include "fsm/fsm.h"

namespace yarn {
	enum ApplicationStateType {
		AS_NEW = 0,
	};

	class Application : public fsm::StateMachine<Application> {
	public:
		Application() : fsm::StateMachine<Application>(ApplicationStateType::AS_NEW) {}
		~Application() {}

		virtual void InitilizeTranform();

	private:
		std::string _name;
		std::set<std::string> _containers;
	};
}

#endif //__APPLICATION_H__
