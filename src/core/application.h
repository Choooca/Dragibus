#pragma once

#include <memory>
#include <GLFW/glfw3.h>

class Window;

namespace Vulkan {
	class Renderer;
}

class Application {

public:
	Application();
	~Application();

	void Loop();

private:

	std::unique_ptr<Window> _window;
	std::unique_ptr<Vulkan::Renderer> _renderer;
	std::unique_ptr<class Scene> _scene;

	void RecreateSwapChainResources();

	static void FramebufferResizedCallback(GLFWwindow* window, int width, int height);
};