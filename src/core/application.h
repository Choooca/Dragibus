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

	std::unique_ptr<Window> m_window;
	std::unique_ptr<Vulkan::Renderer> m_renderer;

	void RecreateSwapChainResources();

	static void FramebufferResizedCallback(GLFWwindow* window, int width, int height);
};