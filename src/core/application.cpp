#include "application.h"
#include <iostream>
#include <render/window.h>
#include <GLFW/glfw3.h>
#include <render/vk_graphic_context.h>

Application::Application()
{
	m_window = new Window();
}

Application::~Application()
{
	
}

void Application::Loop()
{
	GraphicContext graphic_context = CreateGraphicContext(m_window->GetGLFWWindow());

	while (!glfwWindowShouldClose(m_window->GetGLFWWindow())) {
		glfwPollEvents();
	}

	DestroyGraphicContext(graphic_context);
	delete m_window;
}
