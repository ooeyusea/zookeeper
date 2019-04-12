#include "AsyncEventDispatcher.h"

namespace yarn {
	namespace event {
		void AsyncEventDispatcher::Start(int32_t threadCount) {
			_threadCount = threadCount;

			for (int32_t i = 0; i < threadCount; ++i) {
				hn_fork [this]{
					try {
						while (true) {
							EventUnit evt;
							_channel >> evt;

							auto& fns = _functions[evt.typeReflection][evt.type];
							for (auto& fn : fns) {
								fn(evt.e);
							}
						}
					}
					catch (hn_channel_close_exception& e) {

					}

					_closeCh << true;
				};
			}
		}

		void AsyncEventDispatcher::Stop() {
			_channel.Close();

			while (_threadCount > 0) {
				bool re;
				_closeCh >> re;

				--_threadCount;
			}
		}

		void AsyncEventDispatcher::Dispatch(int32_t * key, int64_t type, void * p) {
			try {
				_channel << EventUnit{ key, type, p};
			}
			catch (hn_channel_close_exception& e) {

			}
		}
	}
}
