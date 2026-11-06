#include "input_manager.h"

#include <spdlog/spdlog.h>
#include <utils/debug_macro.h>

InputManager::InputManager(GLFWwindow* window) : _window(window)
{
	_keys.fill(false);
	glfwGetTime();
	_last_time = glfwGetTime();
}

void InputManager::Update()
{
	double time = glfwGetTime();
	_delta_time = time - _last_time;

	for (int i = 0; i < _keys.size(); ++i) {
		int state = glfwGetKey(_window, i + GLFW_KEY_SPACE);
		if (state == GLFW_PRESS) {
			_keys[i] = true;
		}
		else if (state == GLFW_RELEASE) {
			_keys[i] = false;
		}
	}

	glfwGetCursorPos(_window, &_cursor_pos_x, &_cursor_pos_y);

	_cursor_delta_x = _cursor_pos_x - _last_x_cursor_pos;
	_cursor_delta_y = _cursor_pos_y - _last_y_cursor_pos;

	_last_x_cursor_pos = _cursor_pos_x;
	_last_y_cursor_pos = _cursor_pos_y;

	_last_time = time;

	if (_debug) {
		spdlog::debug("delta time = {}", _delta_time);
	}
}

bool InputManager::IsKeyPress(int keyId) const
{
	if (keyId < GLFW_KEY_SPACE || keyId >= GLFW_KEY_LAST) {
		spdlog::error("Input with id : {} doesn't exist.", keyId);
		return false;
	}

	return _keys[keyId - GLFW_KEY_SPACE];
}

bool InputManager::IsMouseButtonPress(int mouseId) const
{
	if (mouseId < GLFW_MOUSE_BUTTON_1 || mouseId > GLFW_MOUSE_BUTTON_LAST) {
		spdlog::error("Mouse Button id : {} doesn't exist.", mouseId);
		return false;
	}
	return glfwGetMouseButton(_window, mouseId) == GLFW_PRESS;
}

double InputManager::GetCursorXDelta() {
	return _cursor_delta_x;
}

double InputManager::GetCursorYDelta() {
	return _cursor_delta_y;
}

double InputManager::GetDeltaTime() {
	return _delta_time;
}