\# ThouArtGame



\## Lightweight, OpenGL-based game engine



\### Overview



Games created in ThouArtGame are driven by a state machine, where states are defined by C++ classes that inherit from the `TAGBaseState` class:



&nbsp;	class MainStateClass : public TAGBaseState {

&nbsp;		...

&nbsp;	}



State classes must implement various functions that allow the state to function as part of the state machine, such as `mainLoop` which runs every frame when the state is the current state.



Then in the source .cpp file:



&nbsp;	int main() {

&nbsp;		TAGBaseState::initGame(TAGBaseState::GameInitializer(1280, 720, "My cool game"));

&nbsp;		TAGBaseState::addState<MainStateClass>("MAIN");

&nbsp;		return TAGBaseState::runGame();

&nbsp;	}



And your good to go!


### External Dependencies

GLAD is used for OpenGL function linking. <https://github.com/Dav1dde/glad>



GLFW is used for window creation and management. <https://github.com/glfw/glfw>



GLM is used for mathematical functions. <https://github.com/g-truc/glm>


STB is used for image loading. <https://github.com/nothings/stb> (specifically stb\_image.h)


tinyobjloader is used for loading .obj files. <https://github.com/tinyobjloader/tinyobjloader>


