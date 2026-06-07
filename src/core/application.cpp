#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include <render/window.h>
#include <render/vk_context.h>
#include <render/vk_render_context.h>
#include <render/vk_render.h>

Application::Application()
{
	m_window = new Window();
}

Application::~Application()
{
	delete m_window;
}

void Application::Loop()
{
	VkContext context = CreateContext(m_window->GetGLFWWindow());
	VkRenderContext render_context = CreateRenderContext(context);

	VkRender* render = new VkRender(&render_context);

	while (!glfwWindowShouldClose(m_window->GetGLFWWindow())) {
		glfwPollEvents();
	}

	delete render;

	DestroyRenderContext(render_context);
	DestroyContext(context);
}
