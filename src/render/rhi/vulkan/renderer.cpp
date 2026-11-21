#include "renderer.h"

#include <ranges>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <utils/files.h>
#include <utils/build_macro.h>
#include <utils/debug_macro.h>
#include <behavior/camera.h>
#include <render/primitives.h>
#include <render/rhi/vulkan/resource_utils.h>
#include <render/rhi/shader_constants.h>
#include <render/rhi/vulkan/command_utils.h>
#include <render/rhi/vulkan/context.h>
#include <render/rhi/vulkan/frame_resources.h>
#include <render/rhi/vulkan/swap_chain_resources.h>
#include <render/rhi/vulkan/model.h>
#include <render/rhi/vulkan/pipeline_utils.h>
#include <render/rhi/vulkan/descriptor_utils.h>
#include <render/rhi/vulkan/descriptor_layouts.h>

 Vulkan::Renderer::Renderer(const Vulkan::Context& context)
	: _context(context)
{
	_swap_chain_ressources = std::make_unique<SwapChainResources>(context);

	// ----------------- Descriptor Set -----------------
	
	_scn_descriptor_set_layout = CreateDescriptorSetLayout(SCN_BINDINGS);
	_obj_descriptor_set_layout = CreateDescriptorSetLayout(OBJ_BINDINGS);
	_mat_descriptor_set_layout = CreateDescriptorSetLayout(MAT_BINDINGS);

	std::array<vk::DescriptorSetLayout, 3> descriptor_set_layout{ 
		_scn_descriptor_set_layout,
		_obj_descriptor_set_layout,
		_mat_descriptor_set_layout 
	};

	// ----------------- Lights -----------------

	DirectionalLight dir_light = {
		.direction = glm::normalize(glm::vec3({ -.2, -.9, -.5 })),
		.color = glm::vec4(1.0, 1.0, 1.0, 1.0)
	};

	_dir_light_buffer = CreateDeviceLocalBuffer(_context, dir_light, vk::BufferUsageFlagBits::eUniformBuffer);

	vk::DescriptorBufferInfo dir_light_info = {
		.buffer = _dir_light_buffer.GetBuffer(),
		.offset = 0,
		.range = sizeof(DirectionalLight)
	};

	_light_descriptor_sets = CreateDescriptorSetGroup(_context, 
		{
			.descriptor_set_layout = _scn_descriptor_set_layout,
			.bindings = SCN_BINDINGS,
			.resources = { { {
					std::vector{ dir_light_info }
			} } }
		}
	);

	vk::PushConstantRange node_push_constant_range = {
		.stageFlags = vk::ShaderStageFlagBits::eVertex,
		.offset = 0,
		.size = sizeof(Render::NodeTransformation)
	};
	std::array<vk::PushConstantRange, 1> push_constant_ranges = {
		node_push_constant_range
	};

	vk::PipelineLayoutCreateInfo pipeline_layout_info{
		.setLayoutCount = static_cast<uint32_t>(descriptor_set_layout.size()),
		.pSetLayouts = descriptor_set_layout.data(),
		.pushConstantRangeCount = push_constant_ranges.size(),
		.pPushConstantRanges = push_constant_ranges.data()
	};

	_pipeline_layout = vk::raii::PipelineLayout(_context.GetDevice(), pipeline_layout_info);
	std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute = Vertex::GetAttributeDescription();
	std::vector<vk::VertexInputBindingDescription> vertex_input_binding = { Vertex::GetBindingDescription() };

	GraphicsPipelineDesc graphics_pipeline_desc = {
		.pipeline_layout = _pipeline_layout,
		.attachment_format = context.GetSurfaceFormat().format,
		.depth_format = _context.GetDepthFormat(),
		.vertex_input_attributes = vertex_input_attribute,
		.vertex_input_bindings = vertex_input_binding,
		.file_name = "simple_shader.spv"
	};

	_graphics_pipeline = CreateGraphicsPipeline(_context, graphics_pipeline_desc);

	// ----------------- Sync -----------------

	_render_finish_semaphore.reserve(_swap_chain_ressources->GetSwapChainImageCount());
	for (int i = 0; i < _swap_chain_ressources->GetSwapChainImageCount(); ++i) {
		_render_finish_semaphore.emplace_back(_context.GetDevice(), vk::SemaphoreCreateInfo());
	}

	_frame_resources = std::make_unique<FrameResources>(context, *this);

	glfwSetWindowUserPointer(_context.GetWindow(), this);
	glfwSetFramebufferSizeCallback(_context.GetWindow(), FramebufferResizedCallback);
}

Vulkan::Renderer::~Renderer()
{

}

void Vulkan::Renderer::RecreateSwapChainResources()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(_context.GetWindow(), &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(_context.GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	_context.GetDevice().waitIdle();

	_swap_chain_ressources = std::make_unique<SwapChainResources>(_context);
}

void Vulkan::Renderer::FramebufferResizedCallback(GLFWwindow* window, int width, int height)
{
	Vulkan::Renderer* renderer = reinterpret_cast<Vulkan::Renderer*>(glfwGetWindowUserPointer(window));
	renderer->_frame_buffer_resized = true;
}

void Vulkan::Renderer::UpdateUniformBuffer(uint32_t current_frame, Camera* camera)
{
	Render::GlobalTransformation global_transform{};
	global_transform.view = glm::lookAt(camera->GetPosition(), camera->GetPosition() + camera->GetForward(), glm::vec3(0, 1, 0));
	global_transform.perspective = glm::perspective(glm::radians(45.0f), _swap_chain_ressources->GetSwapChainExtent().width / (float)_swap_chain_ressources->GetSwapChainExtent().height, 0.001f, 1000.0f);

	memcpy(_frame_resources->GetUniformBufferMappedMemory(current_frame), &global_transform, sizeof(Render::GlobalTransformation));
}

void Vulkan::Renderer::RecordCommandBuffer(uint32_t current_frame, uint32_t swap_chain_image_index)
{
	vk::raii::CommandBuffer& command_buffer = _frame_resources->GetCommandBuffer(current_frame);
	command_buffer.begin({});

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetSwapChainImage(swap_chain_image_index),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		{}, vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor);

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetDepthImage(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::ImageAspectFlagBits::eDepth);

	vk::ClearValue clear_color = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	vk::ClearValue clear_depth = vk::ClearDepthStencilValue{ 1.0, 0 };
	vk::RenderingAttachmentInfo attachment_info = {
		.imageView = _swap_chain_ressources->GetSwapChainImageView(swap_chain_image_index),
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = clear_color
	};

	vk::RenderingAttachmentInfo depth_attachment_info = {
		.imageView = _swap_chain_ressources->GetDepthImageView(),
		.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.clearValue = clear_depth
	};

	vk::RenderingInfo rendering_info = {
		.renderArea = { .offset = { 0, 0 }, .extent = _swap_chain_ressources->GetSwapChainExtent() },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachment_info,
		.pDepthAttachment = &depth_attachment_info
	};

	command_buffer.beginRendering(rendering_info);

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphics_pipeline);

	command_buffer.setViewport(0,
		vk::Viewport(
			0.0f, static_cast<float>(_swap_chain_ressources->GetSwapChainExtent().height),
			static_cast<float>(_swap_chain_ressources->GetSwapChainExtent().width), -static_cast<float>(_swap_chain_ressources->GetSwapChainExtent().height),
			0.0f, 1.0f
		));
	command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swap_chain_ressources->GetSwapChainExtent()));

	vk::DescriptorSet obj_descriptor_set = _frame_resources->GetDescriptorSet(current_frame);
	vk::DescriptorSet light_descriptor_set = _light_descriptor_sets.sets[0];
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 0, light_descriptor_set, nullptr);
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 1, obj_descriptor_set, nullptr);

	for (const Vulkan::Model& model : _models) {
		RenderModel(command_buffer, model);
	}

	command_buffer.endRendering();

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetSwapChainImage(swap_chain_image_index),
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite, {},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe
		, vk::ImageAspectFlagBits::eColor);

	command_buffer.end();
}

void Vulkan::Renderer::RenderModel(vk::raii::CommandBuffer& command_buffer, const Vulkan::Model& model)
{
	RenderScene(command_buffer, model, model.GetDefaultScene());
}

void Vulkan::Renderer::RenderScene(vk::raii::CommandBuffer& command_buffer, const Vulkan::Model& model, const Vulkan::Scene& scene)
{
	for (const auto& node : scene.nodes) {
		RenderNode(command_buffer, model, model.GetNode(node), glm::mat4(1));
	}
}

void Vulkan::Renderer::RenderNode(vk::raii::CommandBuffer& command_buffer, const Vulkan::Model& model, const Vulkan::Node& node, const glm::mat4 &parent_model_mat)
{
	glm::mat4 model_matrix = parent_model_mat * node.model_matrix;

	if (node.mesh != -1) {

		Render::NodeTransformation node_transform = {
			.model = model_matrix,
			.normal = glm::transpose(glm::inverse(model_matrix))
		};

		command_buffer.pushConstants(
			_pipeline_layout,
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(Render::NodeTransformation),
			&node_transform
		);

		RenderMesh(command_buffer, model, model.GetMesh(node.mesh));
	}

	for (const auto& child : node.children) {
		RenderNode(command_buffer, model, model.GetNode(child), model_matrix);
	}
}

void Vulkan::Renderer::RenderMesh(vk::raii::CommandBuffer& command_buffer, const Vulkan::Model& model, const Vulkan::Mesh& mesh)
{
	for (const auto& primitive : mesh.primitives) {
		vk::DescriptorSet descriptor_set = primitive.GetMaterialIndex() != -1 ? model.GetMaterialDescriptor(primitive.GetMaterialIndex()) : model.GetDefaultMaterialDescriptor();
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 2, descriptor_set, nullptr);

		vk::DeviceSize offset = { 0 };
		command_buffer.bindVertexBuffers(0, primitive.GetVertexBuffer().GetBuffer(), offset);
		command_buffer.bindIndexBuffer(primitive.GetIndexBuffer().GetBuffer(), 0, vk::IndexType::eUint32);
		command_buffer.drawIndexed(primitive.GetIndexCount(), 1, 0, 0, 0);
	}
}

vk::raii::DescriptorSetLayout Vulkan::Renderer::CreateDescriptorSetLayout(std::span<const vk::DescriptorSetLayoutBinding> bindings)
{
	vk::DescriptorSetLayoutCreateInfo mat_descriptor_set_layout_create_info = {
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};
	return vk::raii::DescriptorSetLayout(_context.GetDevice(), mat_descriptor_set_layout_create_info);
}

void Vulkan::Renderer::Loop(Camera* camera)
{
	vk::Fence in_flight_fence = _frame_resources->GetInFlightFence(_current_frame);
	auto fence_result = _context.GetDevice().waitForFences(in_flight_fence, VK_TRUE, UINT64_MAX);
	if (fence_result != vk::Result::eSuccess) {
		THROW_RUNTIME_ERROR("Failed to wait for fence");
	}

	auto [result, swap_chain_image_index] = _swap_chain_ressources->GetSwapchain().acquireNextImage(UINT64_MAX, _frame_resources->GetImageAvailableSemaphore(_current_frame), nullptr);

	if (result == vk::Result::eErrorOutOfDateKHR) {
		RecreateSwapChainResources();
		return;
	}
	else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		THROW_RUNTIME_ERROR("Failed to acquire swap chain image");
	}

	_context.GetDevice().resetFences(in_flight_fence);

	UpdateUniformBuffer(_current_frame, camera);

	vk::CommandBuffer command_buffer = _frame_resources->GetCommandBuffer(_current_frame);
	RecordCommandBuffer(_current_frame, swap_chain_image_index);

	vk::Semaphore wait_semaphore[] = { _frame_resources->GetImageAvailableSemaphore(_current_frame) };
	vk::PipelineStageFlags wait_stages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore signal_semaphore[] = { _render_finish_semaphore[swap_chain_image_index] };

	const vk::SubmitInfo submit_info{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphore,
		.pWaitDstStageMask = &wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphore
	};

	_context.GetGraphicsQueue().submit(submit_info, in_flight_fence);

	vk::SwapchainKHR swap_chains[] = { _swap_chain_ressources->GetSwapchain() };

	vk::PresentInfoKHR present_info{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphore,
		.swapchainCount = 1,
		.pSwapchains = swap_chains,
		.pImageIndices = &swap_chain_image_index
	};

	result = _context.GetGraphicsQueue().presentKHR(present_info);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _frame_buffer_resized) {
		_frame_buffer_resized = false;
		RecreateSwapChainResources();
	}
	else if (result != vk::Result::eSuccess){
		THROW_RUNTIME_ERROR("Failed to present swap chain image");
	}

	_current_frame = (_current_frame + 1) % _frame_resources->FRAME_IN_FLIGHT;
}

void Vulkan::Renderer::UpdateScene(const std::vector<Render::Model>& models)
{
	_context.GetDevice().waitIdle();

	_models.clear();

	_models.reserve(models.size());
	for (const auto& model : models) {
		_models.emplace_back(_context, *this, model);
	}
}

vk::DescriptorSetLayout Vulkan::Renderer::GetObjDescriptorSetLayout() const { return _obj_descriptor_set_layout; }
vk::DescriptorSetLayout Vulkan::Renderer::GetMatDescriptorSetLayout() const { return _mat_descriptor_set_layout; }