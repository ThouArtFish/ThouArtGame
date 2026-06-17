#pragma once

#include <BaseStateClass.hpp>
#include <ShaderManagerClass.hpp>

class TestState : public TAGBaseState {
public:
	TestState() {};
	std::string mainLoop() { 
		if (isKeyPressed(GLFW_KEY_ESCAPE)) return "END";
		return "CURRENT"; 
	};
	void enter() {};
	void exit() {};
	void framebufferSizeCallback() {};
	void mouseCallback() {};
	void iconifyCallback() {};
private:
	TAGShaderManager shaders = TAGShaderManager({ .name = "shader", .shader_type = TAGShaderManager::ShaderType::UNINSTANCED_BASIC_DRAW });
};
