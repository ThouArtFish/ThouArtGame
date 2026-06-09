#pragma once

#include <BaseStateClass.hpp>
#include <ShaderManagerClass.hpp>

class TestState : public TAGBaseState {
public:
	TestState() {
		hud.loadImage("images/pineapple.png", {}, "pineapple");
		hud.addQuad({ .position = glm::vec2(0.2f), .dimensions = glm::vec2(0.2f), .image_name = "pineapple"});
	};
	std::string mainLoop() { 
		if (isKeyPressed(GLFW_KEY_ESCAPE)) {
			return "END";
		}
		hud.drawAll(shaders.useShader("shader"));
		return "MAIN"; 
	};
	void enter() {};
	void exit() {};
	void framebufferSizeCallback() {};
	void mouseCallback() {};
	void iconifyCallback() {};
private:
	TAGShaderManager shaders = TAGShaderManager({ .name = "shader", .shader_type = TAGShaderManager::ShaderType::HUD_DRAW });
	TAGHUDManager hud = TAGHUDManager(TAGResourceManager::BufferAccess::STREAM, 1);
};
