#include "window.h"

#include <core/input_manager.h>

Window::Window()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	_window = glfwCreateWindow(WIDTH, HEIGHT, "Dragibus", nullptr, nullptr);

	glfwSetWindowUserPointer(_window, this);

	SetCursorCaptured(true);
}

Window::~Window()
{
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Window::Update(const InputManager& input_manager)
{
	if (_cursor_captured && input_manager.IsKeyPress(GLFW_KEY_ESCAPE)) {
		SetCursorCaptured(false);
	}
	else if (!_cursor_captured && input_manager.IsMouseButtonPress(GLFW_MOUSE_BUTTON_1)) {
		SetCursorCaptured(true);
	}
}

GLFWwindow *Window::GetGLFWWindow()
{
	return _window;
}

void Window::SetCursorCaptured(bool is_captured)
{
	_cursor_captured = is_captured;
	if (_cursor_captured) {
		glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else {
		glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

bool Window::IsCursorCaptured() const { return _cursor_captured; }