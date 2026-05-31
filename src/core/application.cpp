#include "application.h"
#include <iostream>
#include <render/window.h>
#include <GLFW/glfw3.h>
#include <render/vk_graphic_context.h>

Application::Application()
{
	m_window = std::make_unique<Window>();
}

Application::~Application()
{
	
}

void Application::Loop()
{
	GraphicContext graphic_context = CreateGraphicContext();

	while (!glfwWindowShouldClose(m_window->GetGLFWWindow())) {
		glfwPollEvents();
	}

	DestroyGraphicContext(graphic_context);
}
