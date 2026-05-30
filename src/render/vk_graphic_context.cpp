#include "vk_graphic_context.h"
#include <stdexcept>
#include <utils/debug_macro.h>

namespace {
	VkInstance CreateVkInstance() {
		VkInstance out;

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

		if (vkCreateInstance(&instance_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create VkInstance.")
		}
	}
}

GraphicContext CreateGraphicContext()
{
	GraphicContext out{};
	out.m_instance = CreateVkInstance();

	return GraphicContext();
}