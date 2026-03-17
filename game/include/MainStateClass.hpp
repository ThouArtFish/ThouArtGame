#pragma once

#include <algorithm>
#include <map>
#include <iostream>
#include <random>
#include <glm/glm.hpp>
#include <glfw_imp.hpp>
#include <BaseStateClass.hpp>
#include <ShaderManagerClass.hpp>
#include <ModelClass.hpp>
#include <SkyboxClass.hpp>
#include <LightManagerClass.hpp>
#include <WorldModelClass.hpp>
#include <PaintingModelClass.hpp>
#include <UtilClass.hpp>

class MainState : public TAGBaseState {
	public:
		MainState(const std::string& state_name);
		std::string mainLoop();
		void enter();
		void exit();
		void framebufferSizeCallback();
		void mouseCallback();
		void iconifyCallback() {};
	private:
		const float camera_speed = 14.0f;
		const float camera_height = 1.9f;
		const float total_height = 2.0f;
		const float capsule_rad = 0.8f;
		const float grav_accel = -10.0f;
		const float jump_accel = 20.0f;
		const float player_light_fact = 0.0005f;
		const float lamp_light_fact = 0.001f;
		const float sens = 0.001f;
		const float fov = 60.0f;
		const float near = 0.1f;
		const float far = 100.0f;
		const glm::vec3 floor_elevation = glm::vec3(0.0f, -20.0f, 0.0f);
		const glm::vec4 player_light = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
		const glm::vec4 lamp_light = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

		float clock = 0.0f;
		float x_rotation = 0.0f;
		float y_rotation = 0.0f;
		bool just_jumped = false;
		bool end_game = false;
		bool grounded = false;
		float y_comp = 0.0f;

		const glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 lamp_pos;
		glm::vec3 stable_position;

		TAGLightManager light_manager = TAGLightManager(2, TAGLightManager::ChangeFreq::STREAM);

		TAGShaderManager shaders = TAGShaderManager({
			{ "shaders/instanced.vert", "shaders/object_frag.frag", "instanced" },
			{ "shaders/uninstanced.vert", "shaders/object_frag.frag", "uninstanced" },
			{ "shaders/skybox.vert", "shaders/skybox.frag", "skybox" }
		});

		TAGPaintingModel images = TAGPaintingModel(
			{ "images/flat_man.png", "images/pineapple.png" },
			{ TAGTexParam::CLAMP_TO_EDGE_TEX, TAGTexParam::LINEAR_INTERP_PIX, TAGTexParam::LINEAR_INTERP_PIX, false, true },
			TAGMesh::MaterialMod()
		);

		TAGSkybox skybox = TAGSkybox("skybox", { TAGTexParam::CLAMP_TO_EDGE_TEX, TAGTexParam::LINEAR_INTERP_PIX, TAGTexParam::LINEAR_INTERP_PIX, false, false });

		static const inline TAGTexLoader::Params model_params = { TAGTexParam::REPEAT_TEX, TAGTexParam::LINEAR_INTERP_PIX, TAGTexParam::LINEAR_INTERP_PIX, false, true };
		TAGModel lamp = TAGModel(model_params, "lamp.txt");
		TAGWorldModel playground = TAGWorldModel(model_params, "playground.txt");

		glm::vec3 processInput();
		void setPerspectiveMatrix();
		void setCameraMatrix();
};

MainState::MainState(const std::string& state_name) {
	//Set camera position
	camera_position = glm::vec3(0.0f, 2.0f, 0.0f);
	stable_position = camera_position;

	// Create paintings
	glm::vec3 stand = glm::vec3(0, -images.getMesh("flat_man").getBoundingBox().min.y * 2.0f, 0);
	images.changeInstances("flat_man").push_back(
		{
			.position = stand + floor_elevation,
			.scale = 2.0f
		}
	);
	stand = glm::vec3(0, -images.getMesh("pineapple").getBoundingBox().min.y * 3.0f, 0);
	images.changeInstances("pineapple").push_back(
		{
			.position = glm::vec3(4.0f, 0.0f, 0.0f) + stand + floor_elevation,
			.angle = -glm::half_pi<float>(),
			.scale = 3.0f
		}
	);

	// Create playground
	playground.changeInstances().push_back(
		{
			.position = floor_elevation,
			.scale = 3.0f
		}
	);

	// Create lights
	std::vector<TAGLightManager::Light>& lights = light_manager.changeLights();
	lights.emplace_back(glm::vec4(glm::vec3(0), player_light_fact), player_light);
	lights.emplace_back(glm::vec4(lamp_pos, lamp_light_fact), lamp_light);

	// Place lamp
	lamp_pos = glm::vec3(0, -lamp.getMesh("lampion").getBoundingBox().min.y * 0.01f, -3.0f) + floor_elevation;

	setPerspectiveMatrix();
}

std::string MainState::mainLoop() {
	// Check if window has been minimized
	if (iconified) {
		return "CURRENT";
	}

	// Check inputs
	glm::vec3 camera_velocity = processInput();

	// Check if player ended game
	if (end_game) {
		return "END";
	}

	// Check capsule body for collisions with terrain and get floor plane
	const glm::vec3 foot = stable_position - camera_up * camera_height;
	std::vector<Collision::Info> all_collisions = playground.capsuleCollision<Collision::ALL>(foot, camera_up * total_height, capsule_rad);
	const int floor_index = TAGWorldModel::rayCollision<Collision::CLOSEST>(foot, -camera_up, all_collisions, -1.0f);

	// If no floor plane is found then the player is in the air
	grounded = (floor_index >= 0);

	// If grounded, move across floor surface
	if (grounded) {
		camera_velocity = TAGUtil::perpendicularCompNorm(all_collisions[floor_index].plane.frag_plane.normal, camera_velocity);
		if (!just_jumped) { // Reset y change when landing on surface
			y_comp = 0.0f;
		}
	}
	else { // Otherwise, cause the player to fall
		y_comp = glm::max(-30.0f, y_comp + grav_accel * (float)delta_time);
		just_jumped = false;
	}

	// Move across any walls encountered as well
	for (size_t i = 0; i < all_collisions.size(); i++) {
		const TAGMesh::Plane& collision_plane = all_collisions[i].plane.frag_plane;
		if (i != floor_index && glm::dot(collision_plane.normal, camera_velocity) < 0.0f && collision_plane.normal.y >= 0.0f) {
			camera_velocity = TAGUtil::perpendicularCompNorm(collision_plane.normal, camera_velocity);
		}
	}

	// Rotate the man
	for (TAGModel::Object& obj : images.changeInstances("flat_man")) {
		images.faceDirec(camera_position, obj, true);
	}

	// Apply camera position changes
	glm::vec3 bounce = glm::vec3(0);
	if (camera_velocity == glm::vec3(0) || !grounded) {
		const float y_diff = glm::abs(camera_position.y - stable_position.y);
		if (y_diff < 0.01f) {
			camera_position = stable_position;
		}
		else {
			camera_position = glm::mix(camera_position, stable_position, 1.0f - (y_diff / 0.91f));
		}
	}
	else {
		// Bounce while running
		bounce = glm::vec3(0.0f, glm::sin(clock), 0.0f);
		if (clock >= glm::two_pi<float>()) {
			clock -= glm::two_pi<float>();
		}
		else {
			clock += (float)delta_time * 10.0f;
		}
	}

	camera_position += (camera_velocity + bounce + glm::vec3(0.0f, y_comp, 0.0f)) * (float)delta_time;
	stable_position += (camera_velocity + glm::vec3(0.0f, y_comp, 0.0f)) * (float)delta_time;

	// Create "look at" matrix to translate objects to camera view space
	setCameraMatrix();

	// Update lights
	light_manager.changeLights().at(0).a = glm::vec4(camera_position, player_light_fact);

	// Draw game objects
	light_manager.bindShaderData(0);

	TAGShaderManager::Shader shader = shaders.useShader("uninstanced");
	lamp.drawOne(shader, { .position = lamp_pos, .scale = 0.01f }, true);

	shader = shaders.useShader("instanced");
	playground.drawAll(shader, true);
	for (const std::string& mesh_name : images.getMeshNames()) {
		images.drawAll(shader, (mesh_name == "flat_man"), mesh_name);
	}

	light_manager.unbindShaderData(0);

	shader = shaders.useShader("skybox");
	skybox.draw(shader, "skybox");

	return "CURRENT";
}

void MainState::enter() {
	//setWindowFullscreen(TAGEnum::TRUE);
	//setMouseLock(TAGEnum::TRUE);
}

void MainState::exit() {
	setMouseLock(TAGEnum::FALSE);
}

// Window callback functions
void MainState::framebufferSizeCallback() {
	setPerspectiveMatrix();
}

void MainState::mouseCallback() {
	x_rotation = glm::max(-1.57f, glm::min(1.57f, ((float)delta_y * sens) + x_rotation));
	y_rotation += (float)(delta_x) * sens;
	camera_direction.x = glm::cos(y_rotation) * glm::cos(x_rotation);
	camera_direction.y = glm::sin(x_rotation);
	camera_direction.z = glm::sin(y_rotation) * glm::cos(x_rotation);
	camera_direction = glm::normalize(camera_direction);
}

// Game state functions
glm::vec3 MainState::processInput() {
	glm::vec3 camera_velocity = glm::vec3(0);
	const glm::vec3 right = glm::normalize(glm::cross(camera_direction, camera_up));
	const glm::vec3 forward = glm::normalize(glm::cross(camera_up, right));
	if (isKeyPressed(GLFW_KEY_W)) {
		camera_velocity += forward;
	}
	if (isKeyPressed(GLFW_KEY_S)) {
		camera_velocity -= forward;
	}
	if (isKeyPressed(GLFW_KEY_A)) {
		camera_velocity -= right;
	}
	if (isKeyPressed(GLFW_KEY_D)) {
		camera_velocity += right;
	}
	if (isKeyPressed(GLFW_KEY_ESCAPE)) {
		end_game = true;
	}
	if (!isKeyStillPressed(GLFW_KEY_P) && isKeyPressed(GLFW_KEY_P)) {
		setMouseLock(TAGEnum::TOGGLE);
		setWindowFullscreen(TAGEnum::TOGGLE);
	}
	if (grounded && !isKeyStillPressed(GLFW_KEY_SPACE) && isKeyPressed(GLFW_KEY_SPACE)) {
		just_jumped = true;
		y_comp += jump_accel;
	}
	if (camera_velocity != glm::vec3(0)) {
		camera_velocity = glm::normalize(camera_velocity) * camera_speed;
	}
	return camera_velocity;
}

void MainState::setPerspectiveMatrix() {
	const glm::mat4 perspec = glm::perspective(glm::radians(fov), (float)width / (float)height, near, far);
	shaders.useShader("uninstanced").setMatrix4("perspective", perspec);
	shaders.useShader("instanced").setMatrix4("perspective", perspec);
	shaders.useShader("skybox").setMatrix4("perspective", perspec);
}

void MainState::setCameraMatrix() {
	const glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_direction, camera_up);
	shaders.useShader("instanced").setMatrix4("view", view);
	shaders.useShader("uninstanced").setMatrix4("view", view);
	shaders.useShader("skybox").setMatrix4("view", glm::mat4(glm::mat3(view)));
}
