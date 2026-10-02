#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include <behavior/camera.h>

#include <core/input_manager.h>

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
	_input_manager = std::make_unique<InputManager>(_window->GetGLFWWindow());
	_camera = std::make_unique<Camera>(_input_manager.get());
}

Application::~Application()
{
}

void Application::Loop()
{
	while (!glfwWindowShouldClose(_window->GetGLFWWindow())) {
		glfwPollEvents();
		_input_manager->Update();
		_camera->Update();
		_renderer->Loop(_camera.get());
	}

	vkDeviceWaitIdle(_renderer->GetDevice());
}

