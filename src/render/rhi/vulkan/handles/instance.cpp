#include "instance.h"


#include <utils/debug_macro.h>

using namespace Vulkan;

Instance::Instance(const std::vector<const char*>& validation_layers, const std::vector<const char*>& extensions, const VkDebugUtilsMessengerCreateInfoEXT& debug_info) {

	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Dragibus";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Dragibus";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instance_info{};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledLayerCount = validation_layers.size();
	instance_info.ppEnabledLayerNames = validation_layers.data();
	instance_info.enabledExtensionCount = extensions.size();
	instance_info.ppEnabledExtensionNames = extensions.data();
	instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debug_info);

	if (vkCreateInstance(&instance_info, nullptr, &m_instance) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create VkInstance.")
	}
}

Vulkan::Instance::~Instance()
{
	vkDestroyInstance(m_instance, nullptr);
}

VkInstance Vulkan::Instance::Get()
{
	return m_instance;
}
