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
				void * e;
			};
		public:
			AsyncEventDispatcher() {}
			virtual ~AsyncEventDispatcher() {}

			void Start(int32_t threadCount);
			void Stop();

		protected:
			virtual void Dispatch(int32_t * key, int64_t type, void * p);

		private:
			hn_channel<EventUnit, EVENT_DISPATCHER_QUEUE_SIZE> _channel;
			hn_channel<bool, -1> _closeCh;
			int32_t _threadCount;
		};
	}
}

#endif //__EVENTDISPATCHER_H__
