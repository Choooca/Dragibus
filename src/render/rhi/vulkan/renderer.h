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
	class Context;
	class FrameResources;
	class SwapChainResources;
	class PrimitiveBuffer;
	class Model;

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

	class Renderer {

	public:

		Renderer(const Vulkan::Context& context);
		~Renderer();

		void Init();
		void Loop(Camera* camera);
		void UpdateScene(const std::vector<Render::Model>& models);

		vk::DescriptorSetLayout GetObjDescriptorSetLayout() const;
		vk::DescriptorSetLayout GetMatDescriptorSetLayout() const;

		bool _frame_buffer_resized = false;

	private:

		uint32_t _current_frame = 0;

		const Context &_context;

		vk::raii::DescriptorSetLayout _obj_descriptor_set_layout = nullptr;
		vk::raii::DescriptorSetLayout _mat_descriptor_set_layout = nullptr;
		vk::raii::PipelineLayout _pipeline_layout = nullptr;
		vk::raii::Pipeline _graphics_pipeline = nullptr;
		std::vector<vk::raii::Semaphore> _render_finish_semaphore;
		std::unique_ptr<SwapChainResources> _swap_chain_ressources;
		std::unique_ptr<FrameResources> _frame_resources;

		std::vector<Vulkan::Model> _models;

		void RecreateSwapChainResources();
		static void FramebufferResizedCallback(GLFWwindow* window, int width, int height);

		void UpdateUniformBuffer(uint32_t current_frame, Camera *renderer);
		void RecordCommandBuffer(uint32_t current_frame, uint32_t image_index);
	};
}