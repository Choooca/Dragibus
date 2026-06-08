#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include <render/window.h>
#include <render/vk_context.h>
#include <render/swap_chain.h>
#include <render/renderer.h>

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
	VkContext *vk_context = CreateContext(m_window->GetGLFWWindow());
	VkSwapChain *swap_chain = CreateSwapChain(*vk_context);
	Renderer* renderer = CreateRenderer(*vk_context);
	swap_chain->m_framebuffer = CreateFramebuffer(*vk_context, *swap_chain, renderer->m_render_pass);

	while (!glfwWindowShouldClose(m_window->GetGLFWWindow())) {
		glfwPollEvents();
	}

	DestroyRenderer(*vk_context, renderer);
	DestroySwapChain(*vk_context, swap_chain);
	DestroyContext(vk_context);
}
