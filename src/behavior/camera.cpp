#include "camera.h"

#include <algorithm>

#include <spdlog/spdlog.h>
#include <core/input_manager.h>

Camera::Camera(InputManager* input_manager) : _input_manager(input_manager)
{
}

void Camera::Update()
{
	_yaw -= _input_manager->GetCursorXDelta() * _sensitivity;
	_pitch -= _input_manager->GetCursorYDelta() * _sensitivity;

	_pitch = std::clamp(_pitch, -89.0f, 89.0f);

	//theta = yaw, phi = _pitch (y is up)

	_fwd = glm::normalize(
		glm::vec3(
			glm::sin(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch)),
			glm::sin(glm::radians(_pitch)),
			glm::cos(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch))
		)
	);
	
	glm::vec3 right = glm::normalize(glm::cross(_fwd, glm::vec3(0, 1, 0)));

	if (_input_manager->IsKeyPress(GLFW_KEY_W)) {
		_position += _fwd * (float)(_speed * _input_manager->GetDeltaTime());
	}
	if (_input_manager->IsKeyPress(GLFW_KEY_S)) {
		_position -= _fwd * (float)(_speed * _input_manager->GetDeltaTime());
	}
	if (_input_manager->IsKeyPress(GLFW_KEY_D)) {
		_position += right * (float)(_speed * _input_manager->GetDeltaTime());
	}
	if (_input_manager->IsKeyPress(GLFW_KEY_A)) {
		_position -= right * (float)(_speed * _input_manager->GetDeltaTime());
	}

	if (_debug) {
		spdlog::debug("x : {}, y : {}, z : {}", _fwd.x, _fwd.y, _fwd.z);
		spdlog::debug("yaw = {},  pitch = {}", _yaw, _pitch);
	}
}
