#include "window.h"

Window::Window()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	_window = glfwCreateWindow(WIDTH, HEIGHT, "Dragibus", nullptr, nullptr);

	glfwSetWindowUserPointer(_window, this);
}

Window::~Window()
{
	glfwDestroyWindow(_window);
	glfwTerminate();
}

GLFWwindow *Window::GetGLFWWindow()
{
	return _window;
}
