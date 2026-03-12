#pragma once

#include <string>
#include <unordered_map>
#include <concepts>
#include <memory>
#include <iostream>
#include <glad/glad.h>
#include <glfw_imp.hpp>
#include "ResourceManagerClass.hpp"
#include "UtilClass.hpp"

/**
 * The backbone of the game engine. Kicks starts everything by initializing various aspects of the engine. 
 * Also manages the various derived state classes, by providing functions that define the state's function
 * and controlling the main game loop.
 * Global variables accessible by any derived state class are also stored and updated here.
 */
class TAGBaseState {
public:
	/**
	 * Container for passing info required for initializing a game
	 */
	struct GameInitializer {
		const int windowWidth;
		const int windowHeight;
		const int openGLVerMajor = 4;
		const int openGLVerMinor = 6;
		const std::string windowName;
		GameInitializer(const int& width, const int& height, const std::string& name) : windowWidth(width), windowHeight(height), windowName(name) {}
	};
	static inline glm::vec3 camera_position = glm::vec3(0.0f);
	static inline glm::vec3 camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);

	virtual ~TAGBaseState() = default;
	TAGBaseState(const TAGBaseState&) = delete;
	TAGBaseState& operator=(const TAGBaseState&) = delete;

	/**
	 * Add a state class to the state machine. The class must be derived from TAGBaseState.
	 * The first state added is the starting state of the game.
	 * 
	 * @param name A unique identifier for the state.
	 */
	template<class C> static void addState(const std::string& name);
	/**
	 * Removes a state from the state machine. Cannot be the current state.
	 * 
	 * @param name A unique identifier for the state.
	 */
	static void deleteState(const std::string& name);
	/**
	 * Initializes the game using the information provided by game_init. Can be only called once.
	 * 
	 * @param game_init Container for passing info required for initializing a game
	 */
	static void initGame(const GameInitializer& game_init);
	/**
	 * Begins the main game loop. Starts a loop until the game ends, returning 0 on a successful execution
	 * 
	 * @return The end state of the game
	 */
	static int runGame();
private:
	static inline bool frame_ready = false;
	static inline bool game_initialized = false;
	static inline bool game_running = false;
	static inline std::unordered_map<std::string, std::unique_ptr<TAGBaseState>> states;
	static inline std::string current = "BASE";
	static inline std::vector<int> still_pressed;
	static inline GLFWwindow* window = nullptr;

	static void checkStillPressed();
	static void baseFramebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void baseMouseCallback(GLFWwindow* window, double x_pos, double y_pos);
	static void baseIconifyCallback(GLFWwindow* window, int inconified);
protected:
	double frame_interval = 1.0 / 60.0;
	static inline int width = 0;
	static inline int height = 0;
	static inline double last_x = -1.0;
	static inline double last_y = -1.0;
	static inline double current_time = 0.0;
	static inline double delta_time = 0.0;
	static inline double delta_x = 0.0;
	static inline double delta_y = 0.0;
	static inline bool iconified = false;
	static inline bool first_mouse = true;
	
	TAGBaseState() {};
	/**
	 * Returns true if the GLFW key defined is currently being pressed
	 *
	 * @param key The GLFW key
	 * @return True if the key is being pressed, false otherwise
	 */
	static bool isKeyPressed(const int& key);
	/**
	 * If the user checked if the GLFW key was being pressed with isKeyPressed, 
	 * returns true if the key has not been let go since then.
	 * 
	 * @param key The GLFW key.
	 * @return True if the GLFW key is still being pressed since the last check with isKeyPressed. Returns false otherwise.
	 */
	static bool isKeyStillPressed(const int& key);
	/**
	 * Sets the state of the mouse depending on state, TAGEnum::TRUE meaning locked to the centre and hidden,
	 * FALSE for free movement and TOGGLE to switch between states.
	 * 
	 * @param state New state for mouse lock.
	 */
	static void setMouseLock(const TAGEnum& state);
	/**
	 * Sets the fullscreen state of the window, TAGEnum::TRUE for fullscreen, FALSE for windowed and TOGGLE to switch.
	 * 
	 * @param state New state for window.
	 */
	static void setWindowFullscreen(const TAGEnum& state);
	/**
	 * The main game loop for a state. This runs every frame if the derived state is the current state.
	 * The return string determines what happens at the end of every loop:
	 * "CURRENT" - The current state does not change.
	 * "END" - The application ends immediately.
	 * Anything else - The current state switches to the state identified by the string. This is the same string passed in addState.
	 * 
	 * @return Determines what happens before the next main game loop.
	 */
	virtual std::string mainLoop();
	/**
	 * Run when the state is set to the current state. This includes if the state is the starting state.
	 */
	virtual void enter();
	/**
	 * Run when the state is removed from the current state position. This includes when the game ends due to "END" return string.
	 */
	virtual void exit();
	/**
	 * Run whenever the mouse moves.
	 */
	virtual void mouseCallback();
	/**
	 * Run when the size of the window changes
	 */
	virtual void framebufferSizeCallback();
	/**
	 * Run when the window is iconified. Whatever the hell that means.
	 */
	virtual void iconifyCallback();
};

#include "../../src/BaseStateClass.inl"
