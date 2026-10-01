#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Vulkan {

	class Surface {

	public:
		Surface(GLFWwindow* window, const VkInstance &instance);
		~Surface();

		VkSurfaceKHR Get();

	private:

		VkSurfaceKHR _surface = VK_NULL_HANDLE;
		VkInstance _instance = VK_NULL_HANDLE;
	};

}