#include "surface.h"
#include <utils/debug_macro.h>

Vulkan::Surface::Surface(GLFWwindow* window, const VkInstance &instance) 
	: m_instance(instance)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &m_surface) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create surface.");
	}
}

Vulkan::Surface::~Surface()
{
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

VkSurfaceKHR Vulkan::Surface::Get()
{
	return m_surface;
}
