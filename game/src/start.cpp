#include <BaseStateClass.hpp>
#include <MainStateClass.hpp>

int main() {
	TAGBaseState::initGame(TAGBaseState::GameInitializer(1280, 720, "Playground"));
	TAGBaseState::addState<MainState>("MAIN");
	return TAGBaseState::runGame();
}
