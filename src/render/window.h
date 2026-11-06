#pragma once

#include <GLFW/glfw3.h>

class InputManager;

class Window {

public:
	Window();
	Window(const Window &other);
	Window &operator=(const Window &other);
	~Window();

	void Update(const InputManager& input_manager);
	GLFWwindow *GetGLFWWindow();

	bool IsCursorCaptured() const;

private:

	const int HEIGHT = 600, WIDTH = 800;

	GLFWwindow *_window;

	bool _cursor_captured = true;

	void SetCursorCaptured(bool is_captured);
};