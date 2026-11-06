#include "context.h"

#include <ranges>
#include <set>

#include <spdlog/spdlog.h>

#include <utils/debug_macro.h>

Vulkan::Context::Context(GLFWwindow *window)
	: _window(window)
{
	// ----------------- Validation Layer -----------------

	std::vector<char const*> required_layers;
	if (enable_validation_layer) {
		required_layers.assign(_validation_layers.begin(), _validation_layers.end());
	}
	auto layer_properties = _raii_context.enumerateInstanceLayerProperties();
	auto unsupported_layer_it = std::ranges::find_if(required_layers,
		[&layer_properties](auto const& required_layer) {
			return std::ranges::none_of(layer_properties,
				[required_layer](auto const& layer_property) {
					return strcmp(layer_property.layerName, required_layer) == 0; });
		});

	if (unsupported_layer_it != required_layers.end()) {
		THROW_RUNTIME_ERROR("Required layer not supported : {}.", std::string(*unsupported_layer_it));
	}

	// ----------------- Extensions -----------------

	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	_required_extensions = std::vector(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layer) {
		_required_extensions.push_back(vk::EXTDebugUtilsExtensionName);
	}

	std::vector<vk::ExtensionProperties> extension_properties = _raii_context.enumerateInstanceExtensionProperties();
	auto unsupported_extension = std::ranges::find_if(_required_extensions,
		[&extension_properties](auto const& required_extension) {
			return std::ranges::none_of(extension_properties,
				[&required_extension](auto const& extension_property) {
					return strcmp(required_extension, extension_property.extensionName) == 0;
				});
		}
	);
	if (unsupported_extension != _required_extensions.end()) {
		THROW_RUNTIME_ERROR("Asked extensions {} are not supported", *unsupported_extension);
	}

	// ----------------- Instance -----------------

	vk::DebugUtilsMessengerCreateInfoEXT debug_info{
	.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
	.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	.pfnUserCallback = &DebugCallback,
	.pUserData = nullptr
	};

	constexpr vk::ApplicationInfo app_info{
		.pApplicationName = "Dragibus",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Dragibus",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14
	};

	vk::InstanceCreateInfo instance_create_info{
		.pNext = enable_validation_layer ? &debug_info : nullptr,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
		.ppEnabledLayerNames = required_layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(_required_extensions.size()),
		.ppEnabledExtensionNames = _required_extensions.data()
	};

	_instance = vk::raii::Instance(_raii_context, instance_create_info);

	// ----------------- Debug -----------------

	if (enable_validation_layer) {
		_debug_messenger = _instance.createDebugUtilsMessengerEXT(debug_info);
	}

	// ----------------- Surface -----------------

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (glfwCreateWindowSurface(*_instance, _window, nullptr, &surface) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create window surface");
	}
	_surface = vk::raii::SurfaceKHR(_instance, surface);

	// ----------------- Physical Device -----------------

	std::vector<vk::raii::PhysicalDevice> physical_devices = _instance.enumeratePhysicalDevices();
	if (physical_devices.empty()) {
		THROW_RUNTIME_ERROR("Failed to find any GPUs with Vulkan support");
	}

	for (auto const& physical_device : physical_devices) {

		QueueFamilyIndices queue_family_indices = {};

		if (physical_device.getProperties().apiVersion < vk::ApiVersion13) {
			continue;
		}

		std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

		for (uint32_t i = 0; i < queue_family_properties.size(); ++i) {

			vk::QueueFamilyProperties queue_family_property = queue_family_properties[i];

			if (queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics && physical_device.getSurfaceSupportKHR(i, *_surface) && !queue_family_indices.graphics_family)
				queue_family_indices.graphics_family = i;

			if (queue_family_property.queueFlags & vk::QueueFlagBits::eTransfer && !(queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics) && !queue_family_indices.transfer_family)
				queue_family_indices.transfer_family = i;
		}

		if (!queue_family_indices.transfer_family.has_value())
			queue_family_indices.transfer_family = queue_family_indices.graphics_family;

		if (!queue_family_indices.IsComplete())
			continue;

		std::vector<vk::ExtensionProperties> available_physical_device_extensions = physical_device.enumerateDeviceExtensionProperties();
		bool support_all_extensions_required = std::ranges::all_of(
			_required_device_extensions,
			[&available_physical_device_extensions](auto const& required_device_extension) {
				return std::ranges::any_of(available_physical_device_extensions,
					[required_device_extension](auto const& available_physical_device_extension) {
						return strcmp(available_physical_device_extension.extensionName, required_device_extension) == 0;
					});
			}
		);

		if (!support_all_extensions_required)
			continue;

		auto features = physical_device.getFeatures2<vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan13Features>();

		bool supports_required_features = features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
			features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
			features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy;

		if (!supports_required_features)
			continue;

		std::vector<vk::SurfaceFormatKHR> available_formats = physical_device.getSurfaceFormatsKHR(*_surface);
		std::vector<vk::PresentModeKHR> available_present_modes = physical_device.getSurfacePresentModesKHR(*_surface);

		if (available_formats.empty() || available_present_modes.empty())
			continue;

		_available_formats = std::move(available_formats);
		_available_present_modes = std::move(available_present_modes);
		_physical_device = physical_device;
		_queue_family_indices = queue_family_indices;
		break;
	}

	if (_physical_device == nullptr) {
		THROW_RUNTIME_ERROR("Failed to find a suitable physical device");
	}

	// ----------------- Sharing Mode -----------------

	if (_queue_family_indices.graphics_family.value() == _queue_family_indices.transfer_family.value()) {
		_queue_sharing = {
			.sharing_mode = vk::SharingMode::eExclusive
		};
	}
	else {
		_queue_sharing = {
			.sharing_mode = vk::SharingMode::eConcurrent,
			.family_indices = { _queue_family_indices.graphics_family.value(), _queue_family_indices.transfer_family.value() }
		};
	}

	// ----------------- Logical Device -----------------

	float queue_priority = 1.0f;
	std::set<uint32_t> unique_families = {
		_queue_family_indices.graphics_family.value(),
		_queue_family_indices.transfer_family.value(), };
	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
	for (uint32_t family : unique_families)
		queue_create_infos.push_back({ .queueFamilyIndex = family, .queueCount = 1, .pQueuePriorities = &queue_priority });

	vk::StructureChain<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan13Features>
		feature_chain = {
			{ .features = { .samplerAnisotropy = true } },
			{ .shaderDrawParameters = true },
			{ .synchronization2 = true, .dynamicRendering = true }
	};

	vk::DeviceCreateInfo device_create_info{
		.pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
		.pQueueCreateInfos = queue_create_infos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(_required_device_extensions.size()),
		.ppEnabledExtensionNames = _required_device_extensions.data()
	};

	_device = vk::raii::Device(_physical_device, device_create_info);

	// ----------------- Queue -----------------

	_graphics_queue = vk::raii::Queue(_device, _queue_family_indices.graphics_family.value(), 0);
	_transfer_queue = vk::raii::Queue(_device, _queue_family_indices.transfer_family.value(), 0);

	// ----------------- Command Pool -----------------

	vk::CommandPoolCreateInfo graphics_pool_info = {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = _queue_family_indices.graphics_family.value()
	};
	_graphics_command_pool = vk::raii::CommandPool(_device, graphics_pool_info);

	vk::CommandPoolCreateInfo transfer_pool_info = {
		.flags = vk::CommandPoolCreateFlagBits::eTransient,
		.queueFamilyIndex = _queue_family_indices.transfer_family.value()
	};
	_transfer_command_pool = vk::raii::CommandPool(_device, transfer_pool_info);

	// ----------------- Swap Chain -----------------

	auto format_it = std::ranges::find_if(_available_formats,
		[](auto const& available_format) {
			return available_format.format == vk::Format::eB8G8R8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
	_surface_format = format_it != _available_formats.end() ? *format_it : _available_formats[0];
	_depth_format = FindDepthFormat(_physical_device);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Vulkan::Context::DebugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
	vk::DebugUtilsMessageTypeFlagsEXT              type,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		spdlog::error("validation layer: {}", pCallbackData->pMessage);
	else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		spdlog::warn("validation layer: {}", pCallbackData->pMessage);
	else if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
		spdlog::debug("validation layer: {}", pCallbackData->pMessage);

	return VK_FALSE;
}

vk::Format Vulkan::Context::FindSupportedFormat(vk::PhysicalDevice physical_device, const std::vector<vk::Format> candidates, const vk::ImageTiling& tiling, const vk::FormatFeatureFlags& features)
{
	for (const auto format : candidates) {
		vk::FormatProperties properties = physical_device.getFormatProperties(format);

		if (((tiling == vk::ImageTiling::eLinear) && ((properties.linearTilingFeatures & features) == features)) ||
			((tiling == vk::ImageTiling::eOptimal) && ((properties.optimalTilingFeatures & features) == features)))
		{
			return format;
		}
	}

	THROW_RUNTIME_ERROR("Failed to find supported format");
}

vk::Format Vulkan::Context::FindDepthFormat(vk::PhysicalDevice physical_device)
{
	return FindSupportedFormat(physical_device, { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::PhysicalDevice Vulkan::Context::GetPhysicalDevice() const { return _physical_device; }

const Vulkan::QueueSharing& Vulkan::Context::GetQueueSharing() const { return _queue_sharing; }

const vk::raii::Device& Vulkan::Context::GetDevice() const { return _device; }

const vk::raii::SurfaceKHR& Vulkan::Context::GetSurface() const { return _surface; }

const std::vector<vk::PresentModeKHR>& Vulkan::Context::GetAvailablePresentModes() const { return _available_present_modes; }

vk::SurfaceCapabilitiesKHR Vulkan::Context::GetSurfaceCapabilities() const { return _physical_device.getSurfaceCapabilitiesKHR(_surface); }

Vulkan::QueueFamilyIndices Vulkan::Context::GetQueueFamilyIndices() const { return _queue_family_indices; }

vk::Queue Vulkan::Context::GetGraphicsQueue() const { return _graphics_queue; }

vk::Queue Vulkan::Context::GetTransferQueue() const { return _transfer_queue; }

vk::CommandPool Vulkan::Context::GetGraphicsCommandPool() const { return _graphics_command_pool; }

vk::CommandPool Vulkan::Context::GetTransferCommandPool() const { return _transfer_command_pool; }

const vk::SurfaceFormatKHR &Vulkan::Context::GetSurfaceFormat() const { return _surface_format; }

vk::Format Vulkan::Context::GetDepthFormat() const { return _depth_format; }

GLFWwindow* Vulkan::Context::GetWindow() const { return _window; }
