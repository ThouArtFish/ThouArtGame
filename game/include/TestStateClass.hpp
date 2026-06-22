#pragma once

#include <BaseStateClass.hpp>
#include <ShaderManagerClass.hpp>

class TestState : public TAGBaseState {
public:
	TestState() {
		for (GLuint i = 0; i < 3; i++) {
			model.setInstance({ .position = {-5.0f + i * 5.0f, 0.0f, -10.0f}, .scale = 0.03f });
		}
		const auto& shader = shaders.useShader("shader");
		shader.set<glm::mat4>(TAGShaderManager::default_options.view_mat, createCameraMatrix());
		shader.set<glm::mat4>(TAGShaderManager::default_options.perspective_mat, createPerspectiveMatrix());
	};
	std::string mainLoop() { 
		if (isKeyPressed(GLFW_KEY_ESCAPE)) return "END";
		model.drawAll(shaders.useShader("shader"));
		return "CURRENT"; 
	};
	void enter() {};
	void exit() {};
	void framebufferSizeCallback() {};
	void mouseCallback() {};
	void iconifyCallback() {};
private:
	TAGShaderManager shaders = TAGShaderManager({ .name = "shader", .shader_type = TAGShaderManager::ShaderType::INSTANCED_BASIC_DRAW });
	TAGModel model = TAGModel({}, TAGResourceManager::BufferAccess::STATIC, "lamp.txt");
};
