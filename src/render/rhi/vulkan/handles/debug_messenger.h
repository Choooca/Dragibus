#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {
	
	class DebugMessenger {

	public:
		DebugMessenger(const VkInstance &instance, const VkDebugUtilsMessengerCreateInfoEXT &debug_info);
		~DebugMessenger();

	private:

		VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
		VkInstance _instance = VK_NULL_HANDLE;
	};

}