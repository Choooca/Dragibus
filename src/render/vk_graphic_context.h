#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

struct GraphicContext {
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;
	VkPhysicalDevice m_physical_device;
	VkDevice m_device;
	VkSwapchainKHR m_swap_chain;
	VkRenderPass m_render_pass;
};

GraphicContext CreateGraphicContext();
void DestroyGraphicContext(const GraphicContext &ctx);