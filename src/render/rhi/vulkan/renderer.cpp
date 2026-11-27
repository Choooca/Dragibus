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
		.color = glm::vec4(10.0, 10.0, 10.0, 1.0)
	};

	_dir_light_buffer = CreateDeviceLocalBuffer(_context, dir_light, vk::BufferUsageFlagBits::eUniformBuffer);

	vk::DescriptorBufferInfo dir_light_info = {
		.buffer = _dir_light_buffer.GetBuffer(),
		.offset = 0,
		.range = sizeof(DirectionalLight)
	};

	PointLight point_light = {
		.position = { 5.0, 5.0, -3.0 },
		.color = { 1.0, 1.0, 0.0 },
		.intensity = 3
	};

	_point_light_buffer = CreateDeviceLocalBuffer(_context, point_light, vk::BufferUsageFlagBits::eUniformBuffer);

	vk::DescriptorBufferInfo point_light_info = {
		.buffer = _point_light_buffer.GetBuffer(),
		.offset = 0,
		.range = sizeof(PointLight)
	};

	_light_descriptor_sets = CreateDescriptorSetGroup(_context,
		{
			.descriptor_set_layout = _scn_descriptor_set_layout,
			.bindings = SCN_BINDINGS,
			.resources = { { {
					std::vector{ dir_light_info },
					std::vector{ point_light_info }
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

	vk::PipelineLayoutCreateInfo graphics_pipeline_layout_info{
		.setLayoutCount = static_cast<uint32_t>(descriptor_set_layout.size()),
		.pSetLayouts = descriptor_set_layout.data(),
		.pushConstantRangeCount = push_constant_ranges.size(),
		.pPushConstantRanges = push_constant_ranges.data()
	};

	_graphics_pipeline_layout = vk::raii::PipelineLayout(_context.GetDevice(), graphics_pipeline_layout_info);
	std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute = Vertex::GetAttributeDescription();
	std::vector<vk::VertexInputBindingDescription> vertex_input_binding = { Vertex::GetBindingDescription() };

	GraphicsPipelineDesc graphics_pipeline_desc = {
		.pipeline_layout = _graphics_pipeline_layout,
		.attachment_format = HDR_FORMAT,
		.depth_format = _context.GetDepthFormat(),
		.vertex_input_attributes = vertex_input_attribute,
		.vertex_input_bindings = vertex_input_binding,
		.cull_mode = vk::CullModeFlagBits::eBack,
		.file_name = "simple_shader.spv"
	};

	_graphics_pipeline = CreateGraphicsPipeline(_context, graphics_pipeline_desc);

	_tonemap_descriptor_set_layout = CreateDescriptorSetLayout(TONEMAPPING_BINDINGS);

	std::vector<vk::DescriptorSetLayout> tonemap_descriptor_set_layouts = {
		_tonemap_descriptor_set_layout
	};

	vk::PushConstantRange exposure_push_constant_range = {
		.stageFlags = vk::ShaderStageFlagBits::eFragment,
		.offset = 0,
		.size = sizeof(float)
	};
	std::array<vk::PushConstantRange, 1> exposure_push_constant_ranges = {
		exposure_push_constant_range
	};

	vk::PipelineLayoutCreateInfo tonemap_pipeline_layout_info{
		.setLayoutCount = static_cast<uint32_t>(tonemap_descriptor_set_layouts.size()),
		.pSetLayouts = tonemap_descriptor_set_layouts.data(),
		.pushConstantRangeCount = static_cast<uint32_t>(exposure_push_constant_ranges.size()),
		.pPushConstantRanges = exposure_push_constant_ranges.data()
	};

	_tonemap_pipeline_layout = vk::raii::PipelineLayout(_context.GetDevice(), tonemap_pipeline_layout_info);

	GraphicsPipelineDesc tonemap_pipeline_desc = {
		.pipeline_layout = _tonemap_pipeline_layout,
		.attachment_format = context.GetSurfaceFormat().format,
		.depth_format = vk::Format::eUndefined,
		.vertex_input_attributes = {},
		.vertex_input_bindings = {},
		.cull_mode = vk::CullModeFlagBits::eNone,
		.file_name = "tonemap.spv"
	};

	_tonemap_pipeline = CreateGraphicsPipeline(_context, tonemap_pipeline_desc);

	_post_process_sampler = CreateSampler(_context, vk::SamplerAddressMode::eClampToEdge, vk::False);

	_swap_chain_ressources = std::make_unique<SwapChainResources>(context, _post_process_sampler, _tonemap_descriptor_set_layout);

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

	_swap_chain_ressources = std::make_unique<SwapChainResources>(_context, _post_process_sampler, _tonemap_descriptor_set_layout, _swap_chain_ressources->GetSwapchain());
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
	global_transform.view_pos = camera->GetPosition();

	memcpy(_frame_resources->GetUniformBufferMappedMemory(current_frame), &global_transform, sizeof(Render::GlobalTransformation));
}

void Vulkan::Renderer::RecordCommandBuffer(uint32_t current_frame, uint32_t swap_chain_image_index)
{
	vk::raii::CommandBuffer& command_buffer = _frame_resources->GetCommandBuffer(current_frame);
	command_buffer.begin({});

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetOffScreenImage(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		{}, vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eFragmentShader, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor);

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetDepthImage(),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
		vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
		vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
		vk::ImageAspectFlagBits::eDepth);

	vk::ClearValue clear_color = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	vk::ClearValue clear_depth = vk::ClearDepthStencilValue{ 1.0, 0 };

	vk::RenderingAttachmentInfo offscreen_attachment_info = {
		.imageView = _swap_chain_ressources->GetOffScreenImageView(),
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = clear_color
	};

	vk::RenderingAttachmentInfo offscreen_depth_attachment_info = {
		.imageView = _swap_chain_ressources->GetDepthImageView(),
		.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.clearValue = clear_depth
	};

	vk::RenderingInfo offscreen_rendering_info = {
		.renderArea = { .offset = { 0, 0 }, .extent = _swap_chain_ressources->GetSwapChainExtent() },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &offscreen_attachment_info,
		.pDepthAttachment = &offscreen_depth_attachment_info
	};

	command_buffer.beginRendering(offscreen_rendering_info);

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
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _graphics_pipeline_layout, 0, light_descriptor_set, nullptr);
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _graphics_pipeline_layout, 1, obj_descriptor_set, nullptr);

	for (const Vulkan::Model& model : _models) {
		RenderModel(command_buffer, model);
	}

	command_buffer.endRendering();

	Vulkan::CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetOffScreenImage(),
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eShaderRead,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eFragmentShader
		, vk::ImageAspectFlagBits::eColor);

	CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetSwapChainImage(swap_chain_image_index),
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
		{}, vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::ImageAspectFlagBits::eColor);

	vk::RenderingAttachmentInfo tonemap_attachment_info = {
		.imageView = _swap_chain_ressources->GetSwapChainImageView(swap_chain_image_index),
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eDontCare,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = clear_color
	};

	vk::RenderingInfo rendering_info = {
		.renderArea = { .offset = { 0, 0 }, .extent = _swap_chain_ressources->GetSwapChainExtent() },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &tonemap_attachment_info,
	};

	command_buffer.beginRendering(rendering_info);

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _tonemap_pipeline);

	command_buffer.setViewport(0,
		vk::Viewport(
			0.0f, 0.0f,
			_swap_chain_ressources->GetSwapChainExtent().width, _swap_chain_ressources->GetSwapChainExtent().height,
			0.0f, 1.0f
		));
	command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swap_chain_ressources->GetSwapChainExtent()));

	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _tonemap_pipeline_layout, 0, _swap_chain_ressources->GetTonemapDescriptorSet(), nullptr);
	float exposure = 0.6f;
	command_buffer.pushConstants(_tonemap_pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(float), &exposure);
	command_buffer.draw(3, 1, 0, 0);

	command_buffer.endRendering();

	CommandUtils::TransitionImageLayout(
		command_buffer, _swap_chain_ressources->GetSwapChainImage(swap_chain_image_index),
		vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite, {},
		vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eNone,
		vk::ImageAspectFlagBits::eColor);

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
			_graphics_pipeline_layout,
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
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _graphics_pipeline_layout, 2, descriptor_set, nullptr);

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
	vk::Semaphore signal_semaphore[] = { _swap_chain_ressources->GetRenderFinishSemaphore(swap_chain_image_index) };

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