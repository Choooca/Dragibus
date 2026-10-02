#pragma once

#include <memory>
#include <vector>

#include <GLFW/glfw3.h>

#include <utils/custom_type.h>

struct Primitive;
struct Mesh;
class Camera;

namespace Vulkan {

	struct QueueFamilyIndices;
	struct SwapChainSupportDetails;
	class Renderer;
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

	struct GPUPrimitive {

		GPUPrimitive(Renderer* renderer, const Primitive& primitive);

		std::unique_ptr<PrimitiveBuffer> _vertex_buffer;
		std::unique_ptr<PrimitiveBuffer> _index_buffer;
		int _index_count = 0;
	};

	class Renderer {

	public:

		Renderer(GLFWwindow *window);
		~Renderer();

		void Init();
		void Loop(Camera* camera);

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

		void AddScene(const std::vector<Mesh>& meshes);

		bool _frame_buffer_resized = false;

	private:

		uint32_t _current_frame = 0;

		GLFWwindow* _window;

		const std::vector<const char*> _validation_layers{ "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> _extensions;
		const std::vector<const char*> _device_extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDebugUtilsMessengerCreateInfoEXT _debug_info;

		std::unique_ptr<Instance> _instance;
		std::unique_ptr<DebugMessenger> _debug_messenger;
		std::unique_ptr<Surface> _surface;
		VkPhysicalDevice _physical_device;
		QueueFamilyIndices _queue_family_indices;
		std::unique_ptr<Device> _device;
		VkFormat _depth_format;
		VkQueue _graphics_queue;
		VkQueue _transfer_queue;
		VkQueue _present_queue;
		VkSurfaceFormatKHR _surface_format;
		std::unique_ptr<CommandPool> _graphics_command_pool;
		std::unique_ptr<CommandPool> _transfer_command_pool;
		std::unique_ptr<SwapChainResources> _swap_chain_ressources;
		std::unique_ptr<RenderPass> _render_pass;
		std::unique_ptr<DescriptorSetLayout> _descriptor_set_layout;
		std::unique_ptr<GraphicsPipeline> _graphics_pipeline;
		std::unique_ptr<PrimitiveBuffer> _vertex_buffer;
		std::unique_ptr<PrimitiveBuffer> _index_buffer;
		std::unique_ptr<Texture> _texture;
		std::unique_ptr<ImageView> _texture_view;
		std::unique_ptr<Sampler> _sampler;
		std::unique_ptr<FrameResources> _frame_resources;
		std::vector<Semaphore> _render_finish_semaphore;

		std::vector<GPUPrimitive> _gpu_primitives;

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

		void UpdateUniformBuffer(uint32_t current_frame, Camera *renderer);
		void RecordCommandBuffer(uint32_t current_frame, uint32_t image_index);
	};
}