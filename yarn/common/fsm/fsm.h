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
			class Transformer {
				struct Transform {
					int32_t target;
					std::function<bool(T& t, void * e)> check;
				};

			public:
				Transformer() {}
				~Transformer() {}

				template <typename E>
				inline Transformer& AddTransform(int32_t prev, int32_t target, const std::function<bool(T& t, E& e)>& fn = nullptr) {
					auto& transform = _tranforms[prev][&TypeReflection<E>::key];
					transform.target = target;

					if (fn) {
						transform.check = [fn](T& t, void * e) {
							return fn(t, *(static_cast<E*>(e)));
						};
					}
					else
						transform.check = nullptr;

					return *this;
				}

				template <typename E>
				inline Transformer& AddTransform(int32_t prev, int32_t target, bool (T::*fn)(E& e)) {
					auto& transform = _tranforms[prev][&TypeReflection<E>::key];
					transform.target = target;

					if (fn) {
						transform.check = [fn](T& t, void * e) {
							return t.*fn(*(static_cast<E*>(e)));
						};
					}
					else
						transform.check = nullptr;

					return *this;
				}

				template <typename E>
				inline Transformer& AddTransform(int32_t prev, int32_t target, void (T::*fn)(E& e)) {
					auto& transform = _tranforms[prev][&TypeReflection<E>::key];
					transform.target = target;

					if (fn) {
						transform.check = [fn](T& t, void * e) {
							(t.*fn)(*(static_cast<E*>(e)));
							return true;
						};
					}
					else
						transform.check = nullptr;

					return *this;
				}

				template <typename E>
				inline Transformer& AddTransform(int32_t prev, int32_t target, void (T::*fn)()) {
					auto& transform = _tranforms[prev][&TypeReflection<E>::key];
					transform.target = target;

					if (fn) {
						transform.check = [fn](T& t, void * e) {
							(t.*fn)();
							return true;
						};
					}
					else
						transform.check = nullptr;

					return *this;
				}

				template <typename E>
				inline void Handle(StateMachine * stateMachine, E& e) {
					auto itr = _tranforms.find(stateMachine->GetCurrentState());
					if (itr != _tranforms.end()) {
						auto itrTransform = itr->second.find(&TypeReflection<E>::key);
						if (itrTransform != itr->second.end()) {
							if (itrTransform->second.check && !itrTransform->second.check(*(static_cast<T*>(stateMachine)), &e))
								return;

							stateMachine->ChangeState(itrTransform->second.target);
						}
					}
				}

			private:
				std::unordered_map<int32_t, std::unordered_map<int32_t *, Transform>> _tranforms;
			};

		public:
			StateMachine(int32_t initState) : _prevState(initState), _currentState(initState) {
				if (!g_transforInited) {
					InitilizeTranform();
					g_transforInited = true;
				}
			}
			~StateMachine() {}

			inline int32_t GetPrevState() const { return _prevState; }
			inline int32_t GetCurrentState() const { return _currentState; }

			inline void ChangeState(int32_t state) {
				_prevState = _currentState;
				_currentState = state;
			}

			virtual void InitilizeTranform() = 0;

		protected:
			int32_t _prevState;
			int32_t _currentState;

			static bool g_transforInited;
			static Transformer g_transformer;
		};

		template <typename T>
		bool StateMachine<T>::g_transforInited = false;

		template <typename T>
		typename StateMachine<T>::Transformer StateMachine<T>::g_transformer;
	}
}

#endif //__FSM_H__
