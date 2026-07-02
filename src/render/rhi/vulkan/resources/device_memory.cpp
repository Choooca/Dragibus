#include "device_memory.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::DeviceMemory::DeviceMemory(Renderer* renderer, const VkMemoryRequirements& mem_requirements, const VkMemoryPropertyFlags& properties)
	: m_renderer(renderer)
{
	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = FindMemoryType(m_renderer->GetPhysicalDevice(), mem_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_renderer->GetDevice(), &alloc_info, nullptr, &m_device_memory) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to allocate buffer memory")
	}
}

Vulkan::DeviceMemory::~DeviceMemory()
{
	vkFreeMemory(m_renderer->GetDevice(), m_device_memory, nullptr);
}

VkDeviceMemory Vulkan::DeviceMemory::Get()
{
	return m_device_memory;
}

uint32_t Vulkan::DeviceMemory::FindMemoryType(const VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find suitable memory type");
}
