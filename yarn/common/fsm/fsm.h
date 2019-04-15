#ifndef __FSM_H__
#define __FSM_H__
#include "hnet.h"

namespace yarn {
	namespace fsm {
		template <typename T>
		struct TypeReflection {
			static int32_t key;
		};

		template <typename T>
		int32_t TypeReflection<T>::key;

		template <typename T>
		class StateMachine {
			struct Transform {
				int32_t target;
				std::function<bool(T& t, void * e)> check;
			};

		public:
			StateMachine(int32_t initState) : _prevState(initState), _currentState(initState) {}
			~StateMachine() {}

			template <typename E>
			inline StateMachine& AddTransform(int32_t prev, int32_t target, const std::function<bool(T& t, E& e)>& fn = nullptr) {
				auto& transform = _tranforms[prev][&TypeReflection<E>::key];
				transform.target = target;

				if (fn) {
					transform.check = [fn] (void * e) {
						return fn(*(static_cast<E*>(e)));
					}
				}
				else
					transform.check = nullptr;

				return *this;
			}

			template <typename E>
			inline void Handle(T& t, E& e) {
				auto itr = _tranforms.find(_currentState);
				if (itr != _tranforms.end()) {
					auto itrTransform = itr->second.find(&TypeReflection<E>::key);
					if (itrTransform != itr->second.end()) {
						if (itrTransform->second.check && !itrTransform->second.check(t, &e))
							return;

						_prevState = _currentState;
						_currentState = itrTransform->second.target;
					}
				}
			}

		private:
			int32_t _prevState;
			int32_t _currentState;

			std::unordered_map<int32_t, std::unordered_map<int64_t *, Transform>> _tranforms;
		};
	}
}

#endif //__FSM_H__
