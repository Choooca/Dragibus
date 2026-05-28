#include "window.h"

Window::Window()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Dragibus", nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

GLFWwindow *Window::GetGLFWWindow()
{
	return m_window;
}
