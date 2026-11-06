#pragma once

#include <memory>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <utils/custom_type.h>
#include <render/primitives.h>
#include <render/rhi/vulkan/resource_utils.h>


class Camera;

namespace Vulkan {

	class Renderer;
	class Context;
	class FrameResources;
	class SwapChainResources;
	class Model;

	struct DirectionalLight {
		alignas(16) glm::vec3 direction;
		alignas(16) glm::vec4 color;
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

		vk::raii::DescriptorSetLayout _scn_descriptor_set_layout = nullptr;
		vk::raii::DescriptorSetLayout _obj_descriptor_set_layout = nullptr;
		vk::raii::DescriptorSetLayout _mat_descriptor_set_layout = nullptr;
		vk::raii::PipelineLayout _pipeline_layout = nullptr;
		vk::raii::Pipeline _graphics_pipeline = nullptr;
		std::vector<vk::raii::Semaphore> _render_finish_semaphore;
		std::unique_ptr<SwapChainResources> _swap_chain_ressources;
		std::unique_ptr<FrameResources> _frame_resources;

		std::vector<Vulkan::Model> _models;

		vk::raii::DescriptorPool _light_descriptor_pool = nullptr;
		std::vector<vk::raii::DescriptorSet> _light_descriptor_sets;
		DeviceLocalBuffer _dir_light_buffer;

		void RecreateSwapChainResources();
		static void FramebufferResizedCallback(GLFWwindow* window, int width, int height);

		void UpdateUniformBuffer(uint32_t current_frame, Camera *renderer);
		void RecordCommandBuffer(uint32_t current_frame, uint32_t image_index);

		vk::raii::DescriptorSetLayout CreateDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> &bidings);
	};
}