#pragma once 

#include <optional>

#include <glfw/glfw3.h>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
	std::optional<uint32_t> transfer_family;

	bool IsComplete() {
		return graphics_family.has_value() && present_family.has_value();
	}
};

struct VkContext {
	GLFWwindow* m_window;

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physical_device;
	VkDevice m_device;

	QueueFamilyIndices m_indices;

	VkQueue m_graphics_queue;
	VkQueue m_transfer_queue;
	VkQueue m_present_queue;

	VkSurfaceFormatKHR m_swap_chain_image_format;

	VkFormat m_depth_format;
};

VkContext *CreateContext(GLFWwindow *window);
void DestroyContext(VkContext* vk_context);