#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

struct GraphicContext {
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physical_device;
	VkDevice m_device;

	VkQueue m_graphics_queue;
	VkQueue m_transfer_queue;
	VkQueue m_present_queue;
};

GraphicContext CreateGraphicContext(GLFWwindow *window);
void DestroyGraphicContext(const GraphicContext &ctx);