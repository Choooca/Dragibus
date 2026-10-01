#include "surface.h"
#include <utils/debug_macro.h>

Vulkan::Surface::Surface(GLFWwindow* window, const VkInstance &instance) 
	: _instance(instance)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create surface.");
	}
}

Vulkan::Surface::~Surface()
{
	vkDestroySurfaceKHR(_instance, _surface, nullptr);
}

VkSurfaceKHR Vulkan::Surface::Get()
{
	return _surface;
}
