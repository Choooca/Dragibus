#include "device.h"

#include <set>

#include <utils/custom_type.h>
#include <utils/debug_macro.h>

Vulkan::Device::Device(const VkPhysicalDevice& physical_device, const std::vector<const char*>& device_extensions, const std::vector<const char*>& validation_layers, const QueueFamilyIndices& indices)
{
	std::set<uint32_t> unique_queue_family{
			indices.graphics_family.value(),
			indices.transfer_family.value(),
			indices.present_family.value()
	};

	float queue_priority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
	for (uint32_t queue_family : unique_queue_family) {
		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_info.queueCount = 1;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo device_create_info{};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	device_create_info.pQueueCreateInfos = queue_create_infos.data();
	device_create_info.enabledExtensionCount = device_extensions.size();
	device_create_info.ppEnabledExtensionNames = device_extensions.data();
	device_create_info.pEnabledFeatures = &device_features;
	device_create_info.enabledLayerCount = validation_layers.size();
	device_create_info.ppEnabledLayerNames = validation_layers.data();

	if (vkCreateDevice(physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create logical device")
	}
}

Vulkan::Device::~Device()
{
	vkDestroyDevice(m_device, nullptr);
}

VkDevice Vulkan::Device::Get()
{
	return m_device;
}

VkDevice* Vulkan::Device::GetPtr()
{
	return &m_device;
}
