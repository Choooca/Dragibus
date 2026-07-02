#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

namespace Vulkan {

	struct QueueFamilyIndices;

	class Device {

	public:
		Device(const VkPhysicalDevice &physical_device, const std::vector<const char *> &device_extensions, const std::vector<const char *> &validation_layers, const QueueFamilyIndices &indices);
		~Device();

		VkDevice Get();
		VkDevice* GetPtr();

	private:

		VkDevice m_device = VK_NULL_HANDLE;
	};

}