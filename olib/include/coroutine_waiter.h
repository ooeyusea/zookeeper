#ifndef __COROUTINE_WAITER_H__
#define __COROUTINE_WAITER_H__
#include "hnet.h"
#include <chrono>

namespace olib {
	class WaitCo {
	public:
		WaitCo(hn_channel<int8_t, 1> ch) : _waitCh(ch) {}
		~WaitCo() {};

		inline void Wait() const {
			int8_t res;
			_waitCh >> res;
		}

	private:
		hn_channel<int8_t, 1> _waitCh;
	};

	inline WaitCo DoWork(const std::function<void()>& f, int32_t size = 0) {
		hn_channel<int8_t, 1> ch;
		hn_fork [f, ch]() {
			f();
			ch << (int8_t)1;
		};
		return WaitCo(ch);
	}
}

#endif //__COROUTINE_WAITER_H__
