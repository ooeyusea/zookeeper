#ifndef __ASYNCEVENTDISPATCHER_H__
#define __ASYNCEVENTDISPATCHER_H__
#include "hnet.h"
#include "event/EventDispatcher.h"

#ifndef EVENT_DISPATCHER_QUEUE_SIZE
#define EVENT_DISPATCHER_QUEUE_SIZE 1000
#endif

namespace yarn {
	namespace event {
		class AsyncEventDispatcher : public EventDispatcher {
			struct EventUnit {
				int32_t * typeReflection;
				int64_t type;
				EventBase * e;
			};
		public:
			AsyncEventDispatcher() {}
			virtual ~AsyncEventDispatcher() {}

			void Start();
			void Stop();

		protected:
			virtual void Dispatch(int32_t * key, int64_t type, EventBase * p);

		private:
			hn_channel<EventUnit, EVENT_DISPATCHER_QUEUE_SIZE> _channel;
			hn_channel<bool, -1> _closeCh;
		};
	}
}

#endif //__EVENTDISPATCHER_H__
