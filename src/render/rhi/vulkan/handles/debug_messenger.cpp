#include "debug_messenger.h"
#include <utils/debug_macro.h>

Vulkan::DebugMessenger::DebugMessenger(const VkInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT& debug_info)
	: _instance(instance)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func == nullptr) {
		THROW_RUNTIME_ERROR("Failed to find vkCreateDebugUtilsMessengerEXT ProcAddr.");
	}

	if (func(instance, &debug_info, nullptr, &_debug_messenger) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create debug messenger.");
	}
}

Vulkan::DebugMessenger::~DebugMessenger()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(_instance, _debug_messenger, nullptr);
	}
}
