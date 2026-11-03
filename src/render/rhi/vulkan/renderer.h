#pragma once

#include <memory>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <utils/custom_type.h>
#include <render/primitives.h>


class Camera;

namespace Vulkan {

	class Renderer;
	class FrameResources;
	class SwapChainResources;
	class PrimitiveBuffer;

	struct GPUPrimitive {

		GPUPrimitive(Renderer* renderer, const Render::Primitive& primitive);

		std::unique_ptr<PrimitiveBuffer> _vertex_buffer;
		std::unique_ptr<PrimitiveBuffer> _index_buffer;
		int _index_count = 0;
	};

	struct Vertex : Render::Vertex {

		static vk::VertexInputBindingDescription GetBindingDescription() {
			vk::VertexInputBindingDescription out{
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = vk::VertexInputRate::eVertex
			};

			return out;
		}

		static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescription() {
			std::array<vk::VertexInputAttributeDescription, 4> out{};

			out[0].binding = 0;
			out[0].location = 0;
			out[0].format = vk::Format::eR32G32B32Sfloat;
			out[0].offset = offsetof(Vertex, position);

			out[1].binding = 0;
			out[1].location = 1;
			out[1].format = vk::Format::eR32G32B32Sfloat;
			out[1].offset = offsetof(Vertex, normal);

			out[2].binding = 0;
			out[2].location = 2;
			out[2].format = vk::Format::eR32G32B32A32Sfloat;
			out[2].offset = offsetof(Vertex, tangent);

			out[3].binding = 0;
			out[3].location = 3;
			out[3].format = vk::Format::eR32G32Sfloat;;
			out[3].offset = offsetof(Vertex, tex_coord0);

			return out;
		}
	};

	struct QueueSharing {
		vk::SharingMode sharing_mode;
		std::vector<uint32_t> family_indices;
	};

	class Renderer {

	public:

		Renderer(GLFWwindow *window);
		~Renderer();

		void Init();
		void Loop(Camera* camera);

		GLFWwindow* GetWindow();
		vk::PhysicalDevice GetPhysicalDevice() const;
		const QueueSharing& GetQueueSharing() const;
		const vk::raii::Device &GetDevice() const;
		vk::raii::SurfaceKHR &GetSurface();
		vk::SurfaceFormatKHR GetSurfaceFormat();
		std::vector<vk::PresentModeKHR> &GetAvailablePresentModes();
		vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities();
		vk::Format GetDepthFormat();
		QueueFamilyIndices GetQueueFamilyIndices() const;
		vk::Queue GetGraphicsQueue() const;
		vk::Queue GetTransferQueue() const;
		vk::CommandPool GetGraphicsCommandPool() const;
		vk::CommandPool GetTransferCommandPool() const;
		vk::DescriptorSetLayout GetDescriptorSetLayout();

		void AddScene(const std::vector<Render::Mesh>& meshes);

		bool _frame_buffer_resized = false;

	private:

		uint32_t _current_frame = 0;

		GLFWwindow* _window;

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
		vk::SurfaceCapabilitiesKHR _surface_capabilities;
		vk::raii::Device _device = nullptr;
		vk::raii::Queue _graphics_queue = nullptr;
		vk::raii::Queue _transfer_queue = nullptr;
		vk::SurfaceFormatKHR _surface_format;
		vk::raii::DescriptorSetLayout _descriptor_set_layout = nullptr;
		vk::raii::PipelineLayout _pipeline_layout = nullptr;
		vk::raii::Pipeline _graphics_pipeline = nullptr;
		vk::raii::CommandPool _graphics_command_pool = nullptr;
		vk::raii::CommandPool _transfer_command_pool = nullptr;
		std::vector<vk::raii::Semaphore> _render_finish_semaphore;
		vk::raii::Image _texture_image = nullptr;
		vk::raii::DeviceMemory _texture_memory = nullptr;
		vk::raii::ImageView _texture_view = nullptr;
		vk::raii::Sampler _texture_sampler = nullptr;
		QueueFamilyIndices _queue_family_indices;
		
		vk::Format _depth_format;
		std::unique_ptr<SwapChainResources> _swap_chain_ressources;
		std::unique_ptr<FrameResources> _frame_resources;

		std::vector<GPUPrimitive> _gpu_primitives;

		static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
			vk::DebugUtilsMessageTypeFlagsEXT              type,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
	
		vk::Format FindSupportedFormat(vk::PhysicalDevice physical_device, const std::vector<vk::Format> candidates, const vk::ImageTiling& tiling, const vk::FormatFeatureFlags& features);
		vk::Format FindDepthFormat(vk::PhysicalDevice physical_device);

		void RecreateSwapChainResources();
		static void FramebufferResizedCallback(GLFWwindow* window, int width, int height);

		void UpdateUniformBuffer(uint32_t current_frame, Camera *renderer);
		void RecordCommandBuffer(uint32_t current_frame, uint32_t image_index);
	};
}