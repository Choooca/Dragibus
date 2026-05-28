#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {

public:
	Window();
	Window(const Window &other);
	Window &operator=(const Window &other);
	~Window();

	GLFWwindow *GetGLFWWindow();

private:

	const int HEIGHT = 600, WIDTH = 800;

	GLFWwindow *m_window;
};