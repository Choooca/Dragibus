#pragma once
#include <vector>

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Instance {

	public:

		Instance(const std::vector<const char *> &validation_layers, const std::vector<const char *> &extensions, const VkDebugUtilsMessengerCreateInfoEXT &debug_info);
		~Instance();

		VkInstance Get();

	private:

		VkInstance m_instance = VK_NULL_HANDLE;
	};

}