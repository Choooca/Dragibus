#include "debug_messenger.h"
#include <utils/debug_macro.h>

Vulkan::DebugMessenger::DebugMessenger(const VkInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT& debug_info)
	: m_instance(instance)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func == nullptr) {
		THROW_RUNTIME_ERROR("Failed to find vkCreateDebugUtilsMessengerEXT ProcAddr.");
	}

	if (func(instance, &debug_info, nullptr, &m_debug_messenger) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create debug messenger.");
	}
}

Vulkan::DebugMessenger::~DebugMessenger()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(m_instance, m_debug_messenger, nullptr);
	}
}
