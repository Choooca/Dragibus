#pragma once

#include <vector>

#include <optional>

#include <vulkan/vulkan_raii.hpp>
#include <glfw/glfw3.h>

namespace Vulkan {

	struct QueueSharing {
		vk::SharingMode sharing_mode;
		std::vector<uint32_t> family_indices;
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> transfer_family;

		bool IsComplete() const {
			return graphics_family.has_value() && transfer_family.has_value();
		}
	};

	class Context {

	public :

		Context(GLFWwindow* window);

		vk::PhysicalDevice GetPhysicalDevice() const;
		const QueueSharing& GetQueueSharing() const;
		const vk::raii::Device& GetDevice() const;
		const vk::raii::SurfaceKHR& GetSurface() const;
		const std::vector<vk::PresentModeKHR>& GetAvailablePresentModes() const;
		vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities() const;
		QueueFamilyIndices GetQueueFamilyIndices() const;
		vk::Queue GetGraphicsQueue() const;
		vk::Queue GetTransferQueue() const;
		vk::CommandPool GetGraphicsCommandPool() const;
		vk::CommandPool GetTransferCommandPool() const;
		const vk::SurfaceFormatKHR &GetSurfaceFormat() const;
		vk::Format GetDepthFormat() const;
		GLFWwindow* GetWindow() const;

	private:

		GLFWwindow* _window = nullptr;

		const std::vector<const char*> _validation_layers{ "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
		const bool enable_validation_layer = false;
#else
		const bool enable_validation_layer = true;
#endif

		std::vector<const char*> _required_extensions;
		const std::vector<const char*> _required_device_extensions{ vk::KHRSwapchainExtensionName };

		vk::raii::Context _raii_context;
		vk::raii::Instance _instance = nullptr;
		vk::raii::DebugUtilsMessengerEXT _debug_messenger = nullptr;
		vk::raii::SurfaceKHR _surface = nullptr;
		vk::raii::PhysicalDevice _physical_device = nullptr;
		QueueSharing _queue_sharing;
		std::vector<vk::SurfaceFormatKHR> _available_formats;
		std::vector<vk::PresentModeKHR> _available_present_modes;
		vk::raii::Device _device = nullptr;
		vk::raii::Queue _graphics_queue = nullptr;
		vk::raii::Queue _transfer_queue = nullptr;
		vk::raii::CommandPool _graphics_command_pool = nullptr;
		vk::raii::CommandPool _transfer_command_pool = nullptr;
		vk::SurfaceFormatKHR _surface_format;
		vk::Format _depth_format;

		QueueFamilyIndices _queue_family_indices;

		static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
			vk::DebugUtilsMessageTypeFlagsEXT              type,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		vk::Format FindSupportedFormat(vk::PhysicalDevice physical_device, const std::vector<vk::Format> candidates, const vk::ImageTiling& tiling, const vk::FormatFeatureFlags& features);
		vk::Format FindDepthFormat(vk::PhysicalDevice physical_device);
	};
}