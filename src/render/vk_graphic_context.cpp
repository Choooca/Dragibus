#include "vk_graphic_context.h"
#include <stdexcept>
#include <vector>

#include <spdlog/spdlog.h>

#include <utils/debug_macro.h>

namespace {

#pragma region Callback

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			spdlog::error("validation layer: {}", pCallbackData->pMessage);
		else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			spdlog::warn("validation layer: {}", pCallbackData->pMessage);
		else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			spdlog::debug("validation layer: {}", pCallbackData->pMessage);
		else
			spdlog::info("validation layer : {}", pCallbackData->pMessage);

		return VK_FALSE;
	}

#pragma endregion

#pragma region Check

	bool CheckExtensionSupport(std::vector<const char*> extensions) {
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

		for (const char* extension : extensions) {
			bool found = false;

			for (const VkExtensionProperties &available_extension : available_extensions) {
				if (strcmp(available_extension.extensionName, extension) == 0) {
					found = true;
					break;
				}
			}

			if(!found)
				return false;
		}

		return true;
	}

	bool CheckValidationLayerSupport(const std::vector<const char*>& validation_layer) {
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : validation_layer) {
			bool layer_found = false;

			for (const VkLayerProperties& available_layer : available_layers) {
				if (strcmp(available_layer.layerName, layer_name) == 0) {
					layer_found = true;
					break;
				}
			}

			if (!layer_found)
				return false;
		}

		return true;
	}

#pragma endregion

#pragma region Initialization

	VkInstance CreateInstance(const std::vector<const char*> validation_layers, const std::vector<const char*> extensions, const VkDebugUtilsMessengerCreateInfoEXT &debug_info) {

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

		VkInstance out;
		if (vkCreateInstance(&instance_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create VkInstance.")
		}

		return out;
	}

	VkDebugUtilsMessengerEXT CreateDebugMessenger(const VkInstance& instance, const VkDebugUtilsMessengerCreateInfoEXT& debug_info) {

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func == nullptr) {
			THROW_RUNTIME_ERROR("Failed to find vkCreateDebugUtilsMessengerEXT ProcAddr.");
		}

		VkDebugUtilsMessengerEXT out;
		if (func(instance, &debug_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create debug messenger.");
		}
		return out;
	}

	VkSurfaceKHR CreateSurface(const VkInstance &instance, GLFWwindow *window) {
		
		VkSurfaceKHR out;
		if (glfwCreateWindowSurface(instance, window, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create surface.");
		}

		return out;
	}

#pragma endregion 

}

#pragma region Graphic Context

GraphicContext CreateGraphicContext(GLFWwindow* window)
{
	//Layers
	std::vector<const char*> validation_layers{
		"VK_LAYER_KHRONOS_validation"
	};

	if (!CheckValidationLayerSupport(validation_layers)) {
		THROW_RUNTIME_ERROR("Asked validation layers are not supported.");
	}

	//Extensions 
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	if (!CheckExtensionSupport(extensions)) {
		THROW_RUNTIME_ERROR("Asked extensions are not supported");
	}

	//Debug
	VkDebugUtilsMessengerCreateInfoEXT debug_info{};
	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_info.pfnUserCallback = DebugCallback;
	debug_info.pUserData = nullptr;

	GraphicContext out{};
	out.m_instance = CreateInstance(validation_layers, extensions, debug_info);
	out.m_debug_messenger = CreateDebugMessenger(out.m_instance, debug_info);
	out.m_surface = CreateSurface(out.m_instance, window);

	return out;
}

void DestroyGraphicContext(const GraphicContext& ctx)
{
	vkDestroySurfaceKHR(ctx.m_instance, ctx.m_surface, nullptr);

	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx.m_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(ctx.m_instance, ctx.m_debug_messenger, nullptr);
	}

	vkDestroyInstance(ctx.m_instance, nullptr);
}

#pragma endregion