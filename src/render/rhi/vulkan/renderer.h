#pragma once

#include <memory>
#include <vector>

#include <GLFW/glfw3.h>

#include <utils/custom_type.h>

namespace Vulkan {

	struct QueueFamilyIndices;
	struct SwapChainSupportDetails;
	class Instance;
	class DebugMessenger;
	class Surface;
	class Device;
	class FrameResources;
	class SwapChainResources;
	class RenderPass;
	class DescriptorSetLayout;
	class PipelineLayout;
	class GraphicsPipeline;
	class PrimitiveBuffer;
	class Texture;
	class ImageView;
	class Sampler;
	class CommandPool;
	class Semaphore;

	class Renderer {

	public:

		Renderer(GLFWwindow *window);
		~Renderer();

		void Init();
		void Loop();

		GLFWwindow* GetWindow();
		VkPhysicalDevice GetPhysicalDevice();
		VkDevice GetDevice();
		VkSurfaceKHR GetSurface();
		VkSurfaceFormatKHR GetSurfaceFormat();
		VkFormat GetDepthFormat();
		QueueFamilyIndices GetQueueFamilyIndices();
		SwapChainSupportDetails GetSwapChainSupportDetails(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface);
		VkQueue GetGraphicsQueue();
		VkQueue GetTransferQueue();
		CommandPool *GetGraphicsCommandPool();
		CommandPool *GetTransferCommandPool();
		VkRenderPass GetRenderPass();
		VkDescriptorSetLayout GetDescriptorSetLayout();

		bool m_frame_buffer_resized = false;

	private:

		uint32_t m_current_frame = 0;

		GLFWwindow* m_window;

		const std::vector<const char*> m_validation_layers{ "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> m_extensions;
		const std::vector<const char*> m_device_extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDebugUtilsMessengerCreateInfoEXT m_debug_info;

		std::unique_ptr<Instance> m_instance;
		std::unique_ptr<DebugMessenger> m_debug_messenger;
		std::unique_ptr<Surface> m_surface;
		VkPhysicalDevice m_physical_device;
		QueueFamilyIndices m_queue_family_indices;
		std::unique_ptr<Device> m_device;
		VkFormat m_depth_format;
		VkQueue m_graphics_queue;
		VkQueue m_transfer_queue;
		VkQueue m_present_queue;
		VkSurfaceFormatKHR m_surface_format;
		std::unique_ptr<CommandPool> m_graphics_command_pool;
		std::unique_ptr<CommandPool> m_transfer_command_pool;
		std::unique_ptr<SwapChainResources> m_swap_chain_ressources;
		std::unique_ptr<RenderPass> m_render_pass;
		std::unique_ptr<DescriptorSetLayout> m_descriptor_set_layout;
		std::unique_ptr<GraphicsPipeline> m_graphics_pipeline;
		std::unique_ptr<PrimitiveBuffer> m_vertex_buffer;
		std::unique_ptr<PrimitiveBuffer> m_index_buffer;
		std::unique_ptr<Texture> m_texture;
		std::unique_ptr<ImageView> m_texture_view;
		std::unique_ptr<Sampler> m_sampler;
		std::unique_ptr<FrameResources> m_frame_resources;
		std::vector<Semaphore> m_render_finish_semaphore;

		bool CheckValidationLayerSupport(const std::vector<const char*>& validation_layer);
		bool CheckExtensionSupport(std::vector<const char*> extensions);

		VkSurfaceFormatKHR ChooseSwapChainImageFormat(const SwapChainSupportDetails& swap_chain_support_details);

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	
		QueueFamilyIndices FindQueueFamily(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface);

		VkPhysicalDevice PickPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface, const std::vector<const char*> device_extensions, QueueFamilyIndices& indices);
	
		VkFormat FindSupportedFormat(const VkPhysicalDevice& physical_device, const std::vector<VkFormat> candidates, const VkImageTiling& tiling, const VkFormatFeatureFlags& features);
		VkFormat FindDepthFormat(const VkPhysicalDevice& physical_device);

		void RecreateSwapChainResources();
		static void FramebufferResizedCallback(GLFWwindow* window, int width, int height);

		void UpdateUniformBuffer(uint32_t current_frame);
		void RecordCommandBuffer(uint32_t current_frame, uint32_t image_index);
	};
}