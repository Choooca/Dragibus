#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include <render/window.h>
#include <render/vk_context.h>
#include <render/swap_chain.h>
#include <render/renderer.h>
#include <render/render_loop.h>

Application::Application()
{
}

Application::~Application()
{
}

void Application::Loop()
{
	m_window = new Window();
	VkContext *vk_context = CreateContext(m_window->GetGLFWWindow());
	SwapChain *swap_chain = CreateSwapChain(*vk_context);
	Renderer* renderer = CreateRenderer(*vk_context);
	CreateDepthResources(*vk_context, *renderer, swap_chain->m_swap_chain_extent, swap_chain->m_depth_image, swap_chain->m_depth_image_view, swap_chain->m_depth_image_memory);
	swap_chain->m_framebuffers = CreateFramebuffer(*vk_context, *swap_chain, swap_chain->m_depth_image_view, renderer->m_render_pass);

	while (!glfwWindowShouldClose(m_window->GetGLFWWindow())) {
		glfwPollEvents();
		DrawFrame(*vk_context, *swap_chain, *renderer);
	}

	vkDeviceWaitIdle(vk_context->m_device);

	DestroyRenderer(*vk_context, renderer);
	DestroySwapChain(*vk_context, swap_chain);
	DestroyContext(vk_context);
	delete m_window;
}
