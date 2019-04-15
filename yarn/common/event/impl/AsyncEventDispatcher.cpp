#include "AsyncEventDispatcher.h"

namespace yarn {
	namespace event {
		void AsyncEventDispatcher::Start() {
			hn_fork [this]{
				try {
					while (true) {
						EventUnit evt;
						_channel >> evt;

						auto& fns = _functions[evt.typeReflection][evt.type];
						for (auto& fn : fns) {
							fn(evt.e);
						}

						evt.e->Release();
					}
				}
				catch (hn_channel_close_exception& e) {

				}

				_closeCh << true;
			};
		}

		void AsyncEventDispatcher::Stop() {
			_channel.Close();

			bool re;
			_closeCh >> re;
		}

		void AsyncEventDispatcher::Dispatch(int32_t * key, int64_t type, EventBase * p) {
			try {
				_channel << EventUnit{ key, type, p};
			}
			catch (hn_channel_close_exception& e) {

			}
		}
	}
}
