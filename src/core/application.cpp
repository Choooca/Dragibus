#include "application.h"
#include <iostream>
#include <GLFW/glfw3.h>

#include <behavior/camera.h>

#include <core/input_manager.h>

#include <render/window.h>
#include <render/rhi/vulkan/context.h>
#include <render/rhi/vulkan/renderer.h>
#include <render/rhi/gltf_model.h>
#include <render/rhi/scene.h>

Application::Application()
{
	_scene = std::make_unique<Scene>();
	_scene->AddModel(Render::GLTF::ParseGLTFModel("robot/scene.gltf"));
	_window = std::make_unique<Window>();
	_context = std::make_unique<Vulkan::Context>(_window->GetGLFWWindow());
	_renderer = std::make_unique<Vulkan::Renderer>(*(_context.get()));
	_renderer->UpdateScene(_scene->GetModels());
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
		_window->Update(*_input_manager);

		if (_window->IsCursorCaptured())
			_camera->Update();

		_renderer->Loop(_camera.get());
	}

	_context->GetDevice().waitIdle();
}

