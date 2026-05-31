#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

struct GraphicContext {
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;
	VkSurfaceKHR m_surface;
};

GraphicContext CreateGraphicContext(GLFWwindow *window);
void DestroyGraphicContext(const GraphicContext &ctx);