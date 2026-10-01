#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include <render/window.h>
#include <render/rhi/vulkan/renderer.h>
#include <render/rhi/gltf_model.h>
#include <render/rhi/scene.h>

Application::Application()
{
	std::unique_ptr<GLTFModel> model = std::make_unique<GLTFModel>("robot/scene.gltf");
	_scene = std::make_unique<Scene>();
	_scene->SetMeshes(model->GetMeshes());
	_window = std::make_unique<Window>();
	_renderer = std::make_unique<Vulkan::Renderer>(_window->GetGLFWWindow());
	_renderer->AddScene(_scene->GetMeshes());
}

Application::~Application()
{
}

void Application::Loop()
{

	while (!glfwWindowShouldClose(_window->GetGLFWWindow())) {
		glfwPollEvents();
		_renderer->Loop();
	}

	vkDeviceWaitIdle(_renderer->GetDevice());
}

