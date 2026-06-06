#include <BaseStateClass.hpp>
#include <MainStateClass.hpp>
#include <TestStateClass.hpp>

constexpr bool TEST = true;

void chooseMainState() {
	if (TEST) {
		TAGBaseState::addState<TestState>("MAIN");
	}
	else {
		TAGBaseState::addState<MainState>("MAIN");
	}
}

int main() {
	TAGBaseState::initGame(TAGBaseState::GameInitializer(1280, 720, "Playground"));
	chooseMainState();
	return TAGBaseState::runGame();
}
