#pragma once
#include <glm/glm.hpp>

class InputManager;

class Camera {
public:

	Camera(InputManager* input_manager);
	~Camera() = default;

	void Update();

	glm::vec3 GetForward() { return _fwd; }

	glm::vec3 GetPosition() { return _position; }

private:

	InputManager* _input_manager;

	glm::vec3 _position = glm::vec3(0);
	float _speed = 30.0f;
	float _sensitivity = 1.0f;

	glm::vec3 _fwd = glm::vec3(1, 0, 0);

	float _pitch = 0.0f;
	float _yaw = 270.0f;

	bool _debug = false;
};