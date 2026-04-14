#include <BaseStateClass.hpp>

#ifdef _DEBUG
	#define GL_DEBUG(code) code
	void APIENTRY glDebugCallback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		// Ignore non-significant notifications
		if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
			return;

		std::cerr << "OpenGL Debug Message:\n";
		std::cerr << "  Message: " << message << "\n";
		std::cerr << "  Type: ";

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR: std::cerr << "ERROR"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cerr << "DEPRECATED"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: std::cerr << "UNDEFINED"; break;
		case GL_DEBUG_TYPE_PORTABILITY: std::cerr << "PORTABILITY"; break;
		case GL_DEBUG_TYPE_PERFORMANCE: std::cerr << "PERFORMANCE"; break;
		default: std::cerr << "OTHER"; break;
		}

		std::cerr << "\n  Severity: ";
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH: std::cerr << "HIGH"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: std::cerr << "MEDIUM"; break;
		case GL_DEBUG_SEVERITY_LOW: std::cerr << "LOW"; break;
		}

		std::cerr << "\n-----------------------------\n";
	}
#else
	#define GL_DEBUG(code)
#endif

void TAGBaseState::deleteState(const std::string& name) {
	if (name != current) {
		states.erase(name);
	}
}

void TAGBaseState::initGame(const GameInitializer& game_init) {
	// Stop initialization twice
	if (game_initialized) {
		return;
	}
	else {
		game_initialized = true;
	}

	// Initialize some global variables
	width = game_init.window_width;
	height = game_init.window_height;
	last_x = width / 2.0;
	last_y = height / 2.0;

	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, game_init.openGL_version_major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, game_init.openGL_version_minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GL_DEBUG(glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE));

	// Create GLFW window
	window = glfwCreateWindow(width, height, game_init.window_name.c_str(), NULL, NULL);
	glfwMakeContextCurrent(window);

	// Set gamma
	glfwSetGamma(glfwGetPrimaryMonitor(), 1.0f);

	// Set callbacks
	glfwSetFramebufferSizeCallback(window, baseFramebufferSizeCallback);
	glfwSetCursorPosCallback(window, baseMouseCallback);
	glfwSetWindowIconifyCallback(window, baseIconifyCallback);

	// Load GLAD
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// Debug only
	GL_DEBUG(glEnable(GL_DEBUG_OUTPUT));
	GL_DEBUG(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
	GL_DEBUG(glDebugMessageCallback(glDebugCallback, nullptr));

	// Set pixel stride in memory for texture loading
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

	//Set viewport
	glViewport(0, 0, width, height);

	// Enable depth testing globally
	glEnable(GL_DEPTH_TEST);

	// Enable blending globally
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable back-face culling globally
	glEnable(GL_CULL_FACE);

	// Clear screen colour
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Initialise VSync
	glfwSwapInterval(1);
}

int TAGBaseState::runGame() {
	if (!states.empty() && !game_running) {
		current = states.begin()->first;
		states[current]->enter();
		game_running = true;
	}
	else {
		glfwTerminate();
		return 0;
	}
	current_time = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		// Track time
		const double new_time = glfwGetTime();
		delta_time = new_time - current_time;
		if (delta_time > states[current]->frame_interval && frame_ready && !iconified) {
			glfwSwapBuffers(window);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			current_time = new_time;
			frame_ready = false;
		}
		else if (frame_ready || iconified) {
			continue;
		}

		// Check events for next loop
		glfwPollEvents();

		// Check which keys are still being pressed
		checkStillPressed();

		// Execute current state main loop and then handle return
		const std::string end_state = states[current]->mainLoop();
		if (end_state == "END") {
			states[current]->exit();
			glfwSetWindowShouldClose(window, true);
		}
		else if (states.find(end_state) != states.end()) {
			states[current]->exit();
			current = end_state;
			states[current]->enter();
		}
		frame_ready = true;
	}
	TAGResourceManager::clear();
	glfwTerminate();
	return 0;
}

void TAGBaseState::baseFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
	TAGBaseState::width = width;
	TAGBaseState::height = height;
	glViewport(0, 0, width, height);
	states[current]->framebufferSizeCallback();
}

void TAGBaseState::baseMouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
	if (first_mouse) {
		last_x = xposIn;
		last_y = yposIn;
		first_mouse = false;
	}
	else {
		delta_x = xposIn - last_x;
		delta_y = last_y - yposIn;
	}
	last_x = xposIn;
	last_y = yposIn;
	states[current]->mouseCallback();
}

void TAGBaseState::baseIconifyCallback(GLFWwindow* window, int iconified) {
	TAGBaseState::iconified = (iconified == 1);
	states[current]->iconifyCallback();
}

void TAGBaseState::checkStillPressed() {
	for (int i = 0; i < still_pressed.size(); i++) {
		if (glfwGetKey(window, still_pressed[i]) != GLFW_PRESS) {
			still_pressed.erase(still_pressed.begin() + i);
			i--;
		}
	}
}

bool TAGBaseState::isKeyPressed(const int& key) {
	if (glfwGetKey(window, key) == GLFW_PRESS) {
		if (std::find(still_pressed.begin(), still_pressed.end(), key) == still_pressed.end()) {
			still_pressed.push_back(key);
		}
		return true;
	}
	else {
		return false;
	}
}

bool TAGBaseState::isKeyStillPressed(const int& key) {
	return (std::find(still_pressed.begin(), still_pressed.end(), key) != still_pressed.end());
}

void TAGBaseState::setMouseLock(const TAGEnum& state) {
	int mode;
	if (state == TAGEnum::TOGGLE) {
		mode = (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	else {
		mode = (state == TAGEnum::TRUE ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
	glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void TAGBaseState::setWindowFullscreen(const TAGEnum& state) {
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
	if (state == TAGEnum::TOGGLE) {
		glfwSetWindowMonitor(window, (glfwGetWindowMonitor(window) == NULL ? monitor : NULL), 0, 0, vidmode->width, vidmode->height, vidmode->refreshRate);
	}
	else {
		glfwSetWindowMonitor(window, (state == TAGEnum::TRUE ? monitor : NULL), 0, 0, vidmode->width, vidmode->height, vidmode->refreshRate);
	}
}
