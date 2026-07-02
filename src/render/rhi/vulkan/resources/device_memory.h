#pragma once

#include <vulkan/vulkan.hpp>

namespace Vulkan {

	class Renderer;

	class DeviceMemory {

	public:

		DeviceMemory(Renderer *renderer, const VkMemoryRequirements &mem_requirements, const VkMemoryPropertyFlags &properties);
		~DeviceMemory();

		VkDeviceMemory Get();

	private:

		Renderer* m_renderer;
		VkDeviceMemory m_device_memory;

		uint32_t FindMemoryType(const VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);
	};

}