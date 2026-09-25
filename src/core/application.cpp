#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include <render/window.h>
#include <render/rhi/vulkan/renderer.h>
#include <render/rhi/gltf_model.h>

Application::Application()
{
	std::unique_ptr<GLTFModel> model = std::make_unique<GLTFModel>("robot/scene.gltf");
	m_window = std::make_unique<Window>();
	m_renderer = std::make_unique<Vulkan::Renderer>(m_window->GetGLFWWindow());
}

Application::~Application()
{
}

void Application::Loop()
{

	while (!glfwWindowShouldClose(m_window->GetGLFWWindow())) {
		glfwPollEvents();
		m_renderer->Loop();
	}

	vkDeviceWaitIdle(m_renderer->GetDevice());
}

