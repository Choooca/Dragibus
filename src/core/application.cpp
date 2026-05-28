#include "application.h"
#include <iostream>
#include <render/window.h>
#include <GLFW/glfw3.h>

Application::Application()
{
	m_window = std::make_unique<Window>();
}

Application::~Application()
{
	
}

void Application::Loop()
{
	while (!glfwWindowShouldClose(m_window->GetGLFWWindow())) {
		glfwPollEvents();
	}
}
