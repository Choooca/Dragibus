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

 Vulkan::Renderer::Renderer(const Vulkan::Context& context)
	: _context(context)
{
	_swap_chain_ressources = std::make_unique<SwapChainResources>(context);

	// ----------------- Shaders -----------------

	auto shader_code = ReadFile(std::string(SHADERS_DIR) + "simple_shader.spv");
	vk::ShaderModuleCreateInfo shader_module_create_info{ .codeSize = shader_code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(shader_code.data()) };
	vk::raii::ShaderModule shader_module = vk::raii::ShaderModule(_context.GetDevice(), shader_module_create_info);

	vk::PipelineShaderStageCreateInfo vert_shader_stage_info{
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = shader_module,
		.pName = ShaderEntryPoints::VERTEX
	};

	vk::PipelineShaderStageCreateInfo frag_shader_stage_info{
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = shader_module,
		.pName = ShaderEntryPoints::FRAGMENT
	};

	vk::PipelineShaderStageCreateInfo shader_stage_infos[] = {
		vert_shader_stage_info,
		frag_shader_stage_info
	};

	// ----------------- Descriptor Set -----------------
	
	_scn_descriptor_set_layout = CreateDescriptorSetLayout({
		{ .binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment }
		});

	_obj_descriptor_set_layout = CreateDescriptorSetLayout({ 
			{ .binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex } 
		});

	_mat_descriptor_set_layout = CreateDescriptorSetLayout({
				{ .binding = 0, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = Vulkan::Material::MATERIAL_TEXTURE_COUNT, .stageFlags = vk::ShaderStageFlagBits::eFragment },
				{ .binding = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment }
		});

	std::array<vk::DescriptorSetLayout, 3> descriptor_set_layout{ 
		_scn_descriptor_set_layout,
		_obj_descriptor_set_layout,
		_mat_descriptor_set_layout 
	};

	// ----------------- Lights -----------------

	std::array<vk::DescriptorPoolSize, 1> light_pool_sizes = { 
		{
			{ .type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1 }
		} 
	};

	vk::DescriptorPoolCreateInfo light_descriptor_pool_info = {
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = 1,
		.poolSizeCount = static_cast<uint32_t>(light_pool_sizes.size()),
		.pPoolSizes = light_pool_sizes.data()
	};

	_light_descriptor_pool = vk::raii::DescriptorPool(_context.GetDevice(), light_descriptor_pool_info);

	std::array<vk::DescriptorSetLayout, 1> light_descriptor_set_layouts = { _scn_descriptor_set_layout };
	vk::DescriptorSetAllocateInfo alloc_info = {
		.descriptorPool = _light_descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = light_descriptor_set_layouts.data()
	};

	_light_descriptor_sets = _context.GetDevice().allocateDescriptorSets(alloc_info);

	DirectionalLight dir_light = {
		.direction = glm::normalize(glm::vec3({ -.2, -.9, -.5})),
		.color = glm::vec4(1.0, 1.0, 1.0, 1.0)
	};

	_dir_light_buffer = CreateDeviceLocalBuffer(_context, dir_light, vk::BufferUsageFlagBits::eUniformBuffer);

	vk::DescriptorBufferInfo dir_light_info = {
		.buffer = _dir_light_buffer.GetBuffer(),
		.offset = 0,
		.range = sizeof(DirectionalLight)
	};

	std::array<vk::WriteDescriptorSet, 1> light_write_descriptor_set = {
		{
			{
				.dstSet = _light_descriptor_sets.front(),
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo = &dir_light_info
			}
		}
	};

	_context.GetDevice().updateDescriptorSets(light_write_descriptor_set, {});

	// ----------------- Graphics Pipeline -----------------

	std::vector<vk::DynamicState> dynamic_state = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamic_state_info{ .dynamicStateCount = static_cast<uint32_t>(dynamic_state.size()), .pDynamicStates = dynamic_state.data() };

	auto binding_description = Vertex::GetBindingDescription();
	auto attribute_description = Vertex::GetAttributeDescription();
	vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding_description,
		.vertexAttributeDescriptionCount = attribute_description.size(),
		.pVertexAttributeDescriptions = attribute_description.data()
	};
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{ .topology = vk::PrimitiveTopology::eTriangleList };
	vk::PipelineViewportStateCreateInfo viewport_state_info{ .viewportCount = 1, .scissorCount = 1 };
	vk::PipelineRasterizationStateCreateInfo rasterizer_info{ 
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = vk::False,
		.lineWidth = 1.0f };
	vk::PipelineMultisampleStateCreateInfo multisampling_info{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };
	vk::PipelineColorBlendAttachmentState color_blend_attachment_info{
		.blendEnable = vk::False,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
	vk::PipelineColorBlendStateCreateInfo color_blending_info{
		.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &color_blend_attachment_info };
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_info{
		.depthTestEnable = vk::True,
		.depthWriteEnable = vk::True,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = vk::False,
		.stencilTestEnable = vk::False 
	};
	vk::PipelineLayoutCreateInfo pipeline_layout_info{ .setLayoutCount = static_cast<uint32_t>(descriptor_set_layout.size()), .pSetLayouts = descriptor_set_layout.data(), .pushConstantRangeCount = 0};
	_pipeline_layout = vk::raii::PipelineLayout(_context.GetDevice(), pipeline_layout_info);

	vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_info_chain = {
	{ 
		.stageCount = 2,
		.pStages = shader_stage_infos,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly_info,
		.pViewportState = &viewport_state_info,
		.pRasterizationState = &rasterizer_info,
		.pMultisampleState = &multisampling_info,
		.pDepthStencilState = &depth_stencil_info,
		.pColorBlendState = &color_blending_info,
		.pDynamicState = &dynamic_state_info,
		.layout = _pipeline_layout,
		.renderPass = nullptr },
	{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &(_context.GetSurfaceFormat().format), .depthAttachmentFormat = _context.GetDepthFormat()}};
	_graphics_pipeline = vk::raii::Pipeline(_context.GetDevice(), nullptr, pipeline_info_chain.get<vk::GraphicsPipelineCreateInfo>());

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
	static std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();

	float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(now - start).count();

	Render::UniformBufferObject ubo{};
	ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
	ubo.view = glm::lookAt(camera->GetPosition(), camera->GetPosition() + camera->GetForward(), glm::vec3(0, 1, 0));
	ubo.perspective = glm::perspective(glm::radians(45.0f), _swap_chain_ressources->GetSwapChainExtent().width / (float)_swap_chain_ressources->GetSwapChainExtent().height, 0.001f, 1000.0f);
	ubo.normal = glm::transpose(glm::inverse(ubo.model));

	memcpy(_frame_resources->GetUniformBufferMappedMemory(current_frame), &ubo, sizeof(Render::UniformBufferObject));
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
	vk::DescriptorSet light_descriptor_set = _light_descriptor_sets[0];
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 0, light_descriptor_set, nullptr);
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 1, obj_descriptor_set, nullptr);

	for (const Vulkan::Model& model : _models) {
		for (const Vulkan::Mesh& mesh : model.GetMeshes()) {
			for (const Vulkan::Primitive& primitive : mesh.primitives) {

				const Vulkan::Material &mat = primitive.GetMaterialIndex() != -1 ? model.GetMaterials()[primitive.GetMaterialIndex()] : model.GetMaterials().back();
				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 2, mat.descriptor_sets, nullptr);
				vk::DeviceSize offset = { 0 };
				command_buffer.bindVertexBuffers(0, primitive.GetVertexBuffer().GetBuffer(), offset);
				command_buffer.bindIndexBuffer(primitive.GetIndexBuffer().GetBuffer(), 0, vk::IndexType::eUint32);
				command_buffer.drawIndexed(primitive.GetIndexCount(), 1, 0, 0, 0);
			}
		}
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

vk::raii::DescriptorSetLayout Vulkan::Renderer::CreateDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bidings)
{
	vk::DescriptorSetLayoutCreateInfo mat_descriptor_set_layout_create_info = {
		.bindingCount = static_cast<uint32_t>(bidings.size()),
		.pBindings = bidings.data()
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

	vk::CommandBuffer command_buffer = _frame_resources->GetCommandBuffer(_current_frame);
	RecordCommandBuffer(_current_frame, swap_chain_image_index);

	UpdateUniformBuffer(_current_frame, camera);

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