#include "hnet.h"
#include "event/impl/AsyncEventDispatcher.h"

enum TestType {
	TT_a = 0,
	TT_b,
};

enum TestType2 {
	TT1_a = 0,
	TT1_b,
};

struct Test : public yarn::event::Event<TestType, TestType::TT_a> {
	int32_t a = 0;
};

struct Test1 : public yarn::event::Event<TestType2, TestType2::TT1_b> {
	int32_t a = 0;
};

void start(int32_t argc, char ** argv) {
	yarn::event::AsyncEventDispatcher dispatcher;
	dispatcher.Register<Test>([](Test * t) {
		printf("a = %d\n", t->a);
	});

	dispatcher.Register<Test1>([](Test1 * t) {
		printf("a = %d\n", t->a);
	});

	dispatcher.Handle(new Test);
}
