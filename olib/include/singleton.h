#ifndef __SINGLETON_H__
#define __SINGLETON_H__

namespace olib {
	template <typename T>
	struct Singleton {
		inline static T& Instance() {
			static T g_instance;
			return g_instance;
		}
	};
}
#endif //__SINGLETON_H__
