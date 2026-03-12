#pragma once

#include <GLFW/glfw3.h>
#include <algorithm>
#include <map>
#include "../engine/BaseStateClass.h"
#include "../engine/ShaderClass.h"
#include "../engine/ModelClass.h"
#include "../engine/LightManagerClass.h"
#include "../engine/SkyboxClass.h"
#include "../engine/CollisionModelClass.h"
#include "../engine/PaintingClass.h"

class MainState : public BaseState {
	public:
		MainState(Glob* glob, GLFWwindow* window, int fps) : BaseState(glob, window, fps) {
			// Shaders
			shaders["object"] = Shader("shaders/main/main_vert.vert", "shaders/main/object_frag.frag");
			shaders["painting"] = Shader("shaders/main/main_vert.vert", "shaders/main/painting_frag.frag");
			shaders.at("painting").setInt("diffuse", 0);
			shaders["skybox"] = Shader("shaders/skybox/skybox_vert.vert", "shaders/skybox/skybox_frag.frag");
			shaders.at("skybox").setInt("skybox", 0);

			// Load skybox
			skybox = Skybox("assets/skybox/");

			// Load models and pictures
			loaded_models["surface"] = Model("assets/objects/playground/playground.obj", false);
			loaded_paintings["painting"] = Painting("assets/images/flat_man.png", glm::vec3(0, 0, 1), true, false);

			// Spawn surface model
			Object obj = {
				glm::vec3(0.0f, -5.0f, 0.0f),
				glm::vec3(1),
				glm::vec3(0),
				0.0f,
				0.0f,
				2.5f
			};
			loaded_models["surface"].spawned_models.push_back(obj);

			// Set up collisions for surface
			surface = CollisionModel(loaded_models["surface"]);

			// Spawn painting on ground
			glm::vec3 surface_pos = surface.closestRayHit(glm::vec3(3.0f, 0.0f, 0.0f), glm::vec3(0, -1, 0), true).pos;
			obj = {
				surface_pos + loaded_paintings["painting"].stand, 
				glm::vec3(1.0f), 
				glm::vec3(0.0f), 
				0.0f, 
				0.0f, 
				1.0f
			};
			loaded_paintings["painting"].spawned_paintings.push_back(obj);

			//Spawn point lights
			scene = { 0.2f };
			light_manager.lights.push_back({ glm::vec4(glm::vec3(0.0f), dist_factor), glm::vec4(glm::vec3(1.0f), 0.0f) });
			light_manager.lights.push_back({ glm::vec4(glm::vec3(1.0f), dist_factor), glm::vec4(glm::vec3(1.0f), flash_scope) });
			light_manager.initialiseShaderData(&scene);

			// Set perspective matrix
			setPerspectiveMatrix();
		}
		enum EndState mainLoop();
		void enter();
		void exit();
		void framebufferSizeCallback();
		void mouseCallback();
	private:
		const float camera_speed = 6.0f;
		const float camera_height = 1.9f;
		const float total_height = 2.0f;
		const float capsule_rad = 0.3f;
		const float sens = 0.001f;
		const float fov = 60.0f;
		const float near = 0.1f;
		const float far = 100.0f;
		const float jump_pulse = 8.0f;
		const float grav_accel = 10.0f;
		const float dist_factor = 0.08f;
		const float flash_scope = cos(radians(25.0f));

		float x_rotation = 0.0f;
		float y_rotation = 0.0f;
		float grav_comp = 0.0f;
		bool end_game = false;
		bool in_jump = false;
		bool just_jumped = false;
		bool grounded = false;
		bool pressing_p = false;

		glm::vec3 camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 camera_position = glm::vec3(0.0f, 20.0f, 0.0f);

		std::map<std::string, Painting> loaded_paintings;
		std::map<std::string, Model> loaded_models;
		std::map<std::string, Shader> shaders;

		LightManager light_manager;
		CollisionModel surface;
		Skybox skybox;
		SceneInfo scene;

		glm::vec3 processInput();
		void setPerspectiveMatrix();
};
enum EndState MainState::mainLoop() {
	// Check if window has been minimised
	if (glob->iconified) {
		return CURRENT;
	}

	// Check inputs
	glm::vec3 camera_velocity = processInput();
	if (end_game) {
		return END;
	}

	// Check capsule body for collisions with terrain and change velocity accordingly
	std::vector<unsigned int> collision_indices = surface.capsuleVolumeHit(camera_position + camera_up * ((total_height / 2) - camera_height), total_height / 2, capsule_rad);
	RayHit surface_info = surface.closestRayHit(camera_position, -camera_up, true, collision_indices);
	grounded = surface_info.success;
	vec3 barrier_normal = vec3(0);
	if (grounded) {
		camera_velocity = surface.planeParallelComp(camera_velocity, surface_info.index);
		barrier_normal = surface.planes[surface_info.index].normal;
		in_jump = just_jumped;
		grav_comp = 0.0f;
	}
	else {
		grav_comp = min(30.0f, grav_comp + grav_accel * (float)glob->delta_time);
		just_jumped = false;
	}
	if (camera_velocity != glm::vec3(0)) {
		for (unsigned int index : collision_indices) {
			if (index != surface_info.index && glm::dot(camera_velocity, surface.planes[index].normal) < 0) {
				if (glm::dot(barrier_normal, surface.planes[index].normal) >= 0) {
					barrier_normal = surface.planes[index].normal;
					camera_velocity = surface.planeParallelComp(camera_velocity, index);
				}
				else {
					camera_velocity = glm::vec3(0);
					break;
				}
			}
		}
	}

	// Apply camera position changes
	camera_position += (camera_velocity + glm::vec3(0, (in_jump ? jump_pulse - grav_comp : -grav_comp), 0)) * (float)glob->delta_time;

	// Create "look at" matrix to translate objects to camera view space
	glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_direction, camera_up);
	for (std::pair<std::string, Shader> shader : shaders) {
		shader.second.setMatrix4("view", (shader.first != "skybox" ? view : glm::mat4(glm::mat3(view))));
	}

	// Update lights
	ShaderLight camera_flash = { glm::vec4(camera_position, dist_factor), glm::vec4(camera_direction, flash_scope) };
	light_manager.updateShaderLightsSingle(&camera_flash, light_manager.lights.size() - 1);
	
	// Update and draw game objects
	light_manager.bindShaderData();
	for (std::pair<std::string, Painting> pair : loaded_paintings) {
		pair.second.drawAll(shaders.at("painting"), camera_position);
	}
	glEnable(GL_CULL_FACE);
	for (std::pair<std::string, Model> pair : loaded_models) {
		for (unsigned int i = 0; i < pair.second.spawned_models.size(); i++) {
			Object current_obj = pair.second.spawned_models[i];

			current_obj.position += current_obj.velocity * (float)glob->delta_time;
			current_obj.total_rad += current_obj.rads * (float)glob->delta_time;
			loaded_models[pair.first].spawned_models[i] = current_obj;

			glm::mat4 model = glm::translate(glm::mat4(1.0f), current_obj.position);
			model = glm::rotate(model, current_obj.total_rad, current_obj.rotation_axis);
			shaders.at("object").setMatrix3("normal", glm::mat3(model));
			model = model * glm::mat4(glm::mat3(current_obj.scale));
			shaders.at("object").setMatrix4("model", model);

			pair.second.draw(shaders.at("object"));
		}
	}
	light_manager.unbindShaderData();

	// Draw skybox
	skybox.draw(shaders.at("skybox"));

	glDisable(GL_CULL_FACE);
	return CURRENT;
}
void MainState::enter() {
	// Lock cursor and hide it
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void MainState::exit() {
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

// Window callback functions
void MainState::framebufferSizeCallback() {
	glViewport(0, 0, glob->width, glob->height);
	setPerspectiveMatrix();
}
void MainState::mouseCallback() {
	x_rotation = glm::max(-1.57f, glm::min(1.57f, (glob->delta_y * sens) + x_rotation));
	y_rotation += glob->delta_x * sens;
	camera_direction.x = glm::cos(y_rotation) * glm::cos(x_rotation);
	camera_direction.y = glm::sin(x_rotation);
	camera_direction.z = glm::sin(y_rotation) * glm::cos(x_rotation);
	camera_direction = glm::normalize(camera_direction);
}

// Game state functions
glm::vec3 MainState::processInput() {
	glm::vec3 camera_velocity = glm::vec3(0);
	glm::vec3 right = glm::normalize(glm::cross(camera_direction, camera_up));
	glm::vec3 forward = glm::normalize(glm::cross(camera_up, right));
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera_velocity += forward;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera_velocity -= forward;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera_velocity -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera_velocity += right;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && grounded && !just_jumped) {
		just_jumped = true;
		in_jump = true;
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		end_game = true;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pressing_p) {
		if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glob->first_mouse = true;
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		pressing_p = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && pressing_p) {
		pressing_p = false;
	}
	if (camera_velocity != glm::vec3(0)) {
		camera_velocity = glm::normalize(camera_velocity) * camera_speed * (grounded ? 1.0f : 0.7f);
	}
	return camera_velocity;
}

void MainState::setPerspectiveMatrix() {
	glm::mat4 current_perspec = glm::perspective(glm::radians(fov), (float)glob->width / (float)glob->height, near, far);
	for (std::pair<std::string, Shader> shader : shaders) {
		shader.second.setMatrix4("perspective", current_perspec);
	}
}
