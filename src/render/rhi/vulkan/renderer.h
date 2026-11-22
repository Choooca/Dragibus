#pragma once

#include <memory>
#include <vector>
#include <span>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <utils/custom_type.h>
#include <render/primitives.h>
#include <render/rhi/vulkan/resource_utils.h>
#include <render/rhi/vulkan/descriptor_utils.h>

class Camera;

namespace Vulkan {

	class Renderer;
	class Context;
	class FrameResources;
	class SwapChainResources;
	class Model;
	struct Node;
	struct Mesh;
	struct Scene;

	struct DirectionalLight {
		alignas(16) glm::vec3 direction;
		alignas(16) glm::vec4 color;
	};

	class Renderer {

	public:

		static constexpr vk::Format HDR_FORMAT = vk::Format::eR16G16B16A16Sfloat;

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

		vk::raii::DescriptorSetLayout _scn_descriptor_set_layout = nullptr;
		vk::raii::DescriptorSetLayout _obj_descriptor_set_layout = nullptr;
		vk::raii::DescriptorSetLayout _mat_descriptor_set_layout = nullptr;
		vk::raii::DescriptorSetLayout _tonemap_descriptor_set_layout = nullptr;
		vk::raii::PipelineLayout _graphics_pipeline_layout = nullptr;
		vk::raii::PipelineLayout _tonemap_pipeline_layout = nullptr;
		vk::raii::Pipeline _graphics_pipeline = nullptr;
		vk::raii::Pipeline _tonemap_pipeline = nullptr;
		std::unique_ptr<SwapChainResources> _swap_chain_ressources;
		std::unique_ptr<FrameResources> _frame_resources;

		std::vector<Vulkan::Model> _models;

		vk::raii::Sampler _post_process_sampler = nullptr;

		DescriptorSetGroup _light_descriptor_sets;
		DeviceLocalBuffer _dir_light_buffer;

		void RecreateSwapChainResources();
		static void FramebufferResizedCallback(GLFWwindow* window, int width, int height);

		void UpdateUniformBuffer(uint32_t current_frame, Camera *renderer);
		void RecordCommandBuffer(uint32_t current_frame, uint32_t image_index);

		void RenderModel(vk::raii::CommandBuffer &command_buffer, const Vulkan::Model &model);
		void RenderScene(vk::raii::CommandBuffer& command_buffer, const Vulkan::Model& model, const Vulkan::Scene& scene);
		void RenderNode(vk::raii::CommandBuffer &command_buffer, const Vulkan::Model &model, const Vulkan::Node &node, const glm::mat4& parent_model_mat);
		void RenderMesh(vk::raii::CommandBuffer& command_buffer, const Vulkan::Model& model, const Vulkan::Mesh& mesh);

		vk::raii::DescriptorSetLayout CreateDescriptorSetLayout(std::span<const vk::DescriptorSetLayoutBinding> bindings);
	};
}