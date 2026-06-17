#include <BaseStateClass.hpp>
#include <MainStateClass.hpp>
#include <TestStateClass.hpp>

constexpr bool TEST = true;

int main() {
	TAGBaseState::initGame(TAGBaseState::GameInitializer(1280, 720, "Playground"));
	if (TEST) {
		TAGBaseState::addState<TestState>("MAIN");
	}
	else {
		TAGBaseState::addState<MainState>("MAIN");
	}
	return TAGBaseState::runGame();
}
