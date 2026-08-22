#include <BaseStateClass.hpp>
#include <MainStateClass.hpp>
#include <TestStateClass.hpp>

int main() {
	TAGBaseState::initGame(TAGBaseState::GameInitializer(1280, 720, "Playground"));
	TAGBaseState::addState<TestState>("MAIN");
	return TAGBaseState::runGame();
}
