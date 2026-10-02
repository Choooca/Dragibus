#pragma once

#include <array>

#include <glfw/glfw3.h>


class InputManager {

public:

	InputManager(GLFWwindow* window);
	~InputManager() = default;

	void Update();

	double GetDeltaTime();

	double GetCursorXDelta();
	double GetCursorYDelta();

	bool IsKeyPress(int keyId);

private:

	GLFWwindow* _window;

	std::array<bool, GLFW_KEY_LAST - GLFW_KEY_SPACE> _keys;
	double _last_time = 0, _delta_time = 0;

	double _cursor_pos_x = 0, _cursor_pos_y = 0;
	double _cursor_delta_x = 0, _cursor_delta_y = 0;
	double _last_x_cursor_pos = 0, _last_y_cursor_pos = 0;

	bool _debug = false;
};