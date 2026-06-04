#include "vk_context.h"

#include <stdexcept>
#include <vector>
#include <set>

#include <spdlog/spdlog.h>

#include <utils/debug_macro.h>
#include <render/vk_utils.h>

namespace {

#pragma region Utils

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

	QueueFamilyIndices FindQueueFamily(const VkPhysicalDevice &physical_device, const VkSurfaceKHR &surface) {
		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families_properties(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families_properties.data());

		QueueFamilyIndices indices = {};
		for (size_t i = 0; i < queue_family_count; ++i) {
			VkQueueFamilyProperties queue_family_properties = queue_families_properties[i];

			if (queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphics_family = i;
			}

			if (queue_family_properties.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				indices.transfer_family = i;
			}

			VkBool32 support_surface = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &support_surface);

			if (support_surface) {
				indices.present_family = i;
			}
		}

		if (!indices.transfer_family.has_value()) {
			indices.transfer_family = indices.graphics_family;
		}

		return indices;
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

	VkPhysicalDevice PickPhysicalDevice(const VkInstance &instance, const VkSurfaceKHR &surface, const std::vector<const char*> device_extensions, QueueFamilyIndices &indices) {
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

		if (device_count == 0) {
			THROW_RUNTIME_ERROR("No device found");
		}

		std::vector<VkPhysicalDevice> physical_devices(device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());

		indices = {};
		VkPhysicalDevice out;

		for (VkPhysicalDevice physical_device : physical_devices) {
			
			indices = FindQueueFamily(physical_device, surface);

			uint32_t extension_count;
			vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

			std::vector<VkExtensionProperties> available_extensions(extension_count);
			vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

			std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
			for (const VkExtensionProperties &available_extension : available_extensions) {
				required_extensions.erase(available_extension.extensionName);
			}

			bool extensions_supported = required_extensions.empty();

			SwapChainSupportDetails swap_chain_support_details = GetSwapChainSupportDetails(physical_device, surface);

			bool swap_chain_adequate = false;
			if (extensions_supported) {
				swap_chain_adequate = !swap_chain_support_details.formats.empty() && !swap_chain_support_details.present_modes.empty();
			}

			VkPhysicalDeviceFeatures supported_features;
			vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

			if (indices.IsComplete() && extensions_supported && swap_chain_adequate && supported_features.samplerAnisotropy) {
				return physical_device;
			}
		}

		THROW_RUNTIME_ERROR("Failed to find suitable physical device");
	}

	VkDevice CreateLogicalDevice(const VkPhysicalDevice &physical_device, const QueueFamilyIndices &indices, const std::vector<const char*> &device_extensions, const std::vector<const char*> &validation_layer) {
		
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
		device_create_info.enabledLayerCount = validation_layer.size();
		device_create_info.ppEnabledLayerNames = validation_layer.data();

		VkDevice out;
		if (vkCreateDevice(physical_device, &device_create_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create logical device")
		}

		return out;
	}

#pragma endregion 

}

#pragma region Graphic Context

VkContext CreateContext(GLFWwindow* window)
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
	std::vector<const char*> device_extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

	//Graphic Context
	VkContext out{};
	out.m_window = window;

	out.m_instance = CreateInstance(validation_layers, extensions, debug_info);
	out.m_debug_messenger = CreateDebugMessenger(out.m_instance, debug_info);
	out.m_surface = CreateSurface(out.m_instance, out.m_window);
	out.m_physical_device = PickPhysicalDevice(out.m_instance, out.m_surface, device_extensions, out.m_indices);
	out.m_device = CreateLogicalDevice(out.m_physical_device, out.m_indices, device_extensions, validation_layers);
	
	vkGetDeviceQueue(out.m_device, out.m_indices.graphics_family.value(), 0, &out.m_graphics_queue);
	vkGetDeviceQueue(out.m_device, out.m_indices.present_family.value(), 0, &out.m_present_queue);
	vkGetDeviceQueue(out.m_device, out.m_indices.transfer_family.value(), 0, &out.m_transfer_queue);

	return out;
}

void DestroyContext(const VkContext& ctx)
{
	vkDestroyDevice(ctx.m_device, nullptr);

	vkDestroySurfaceKHR(ctx.m_instance, ctx.m_surface, nullptr);

	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx.m_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(ctx.m_instance, ctx.m_debug_messenger, nullptr);
	}

	vkDestroyInstance(ctx.m_instance, nullptr);
}

#pragma endregion