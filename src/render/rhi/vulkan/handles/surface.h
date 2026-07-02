#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Vulkan {

	class Surface {

	public:
		Surface(GLFWwindow* window, const VkInstance &instance);
		~Surface();

		VkSurfaceKHR Get();

	private:

		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkInstance m_instance = VK_NULL_HANDLE;
	};

}