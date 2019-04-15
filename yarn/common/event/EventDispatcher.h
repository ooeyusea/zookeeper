#ifndef __EVENTDISPATCHER_H__
#define __EVENTDISPATCHER_H__
#include "hnet.h"

namespace yarn {
	namespace event {
		template <typename T>
		struct TypeReflection {
			static int32_t key;
		};

		template <typename T> 
		int32_t TypeReflection<T>::key;

		struct EventBase {
			virtual ~EventBase() {}

			virtual void Release() = 0;
		};

		template <typename T, T t, typename E>
		struct Event : public EventBase {
			virtual ~Event() {}

			typedef T event_type;
			static const event_type type = t;

			E e;

			template <typename... Args>
			Event(Args... args) : e(args...) {}

			template <typename... Args>
			static Event * Create(Args... args) { new Event(args...); }

			virtual void Release() { delete this; }
		};

		class EventDispatcher {
		public:
			EventDispatcher() {}
			virtual ~EventDispatcher() {}

			template <typename E> 
			void Register(const std::function<void (E * e)>& fn) {
				printf("%lld\n", (int64_t)&TypeReflection<typename E::event_type>::key);
				_functions[&TypeReflection<typename E::event_type>::key][(int64_t)E::type].push_back([fn](EventBase * e) {
					fn(static_cast<E*>(e));
				});
			}

			template <typename E>
			void Handle(E * e) {
				Dispatch(&TypeReflection<typename E::event_type>::key, (int64_t)E::type, e);
			}

		protected:
			virtual void Dispatch(int32_t * key, int64_t type, EventBase * p) = 0;

		protected:
			std::unordered_map<int32_t *, std::unordered_map<int32_t, std::vector<std::function<void(EventBase * e)>>>> _functions;
		};
	}
}

#endif //__EVENTDISPATCHER_H__
