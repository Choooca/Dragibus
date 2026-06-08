#include "renderer.h"

#include <array>

#include <render/vk_context.h>
#include <utils/debug_macro.h>
#include <render/primitives.h>
#include <utils/files.h>
#include <utils/build_macro.h>
#include <render/vk_resources.h>

namespace {

#pragma region Types

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 perspective;
	};

#pragma endregion

#pragma region Primitives Memory

	void CreateUniformBuffer(const VkContext &vk_context, std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& device_memory, std::vector<void*>& mapped_memory)
	{
		VkDeviceSize size = sizeof(UniformBufferObject);

		buffers.resize(MAX_FRAMES_IN_FLIGHT);
		device_memory.resize(MAX_FRAMES_IN_FLIGHT);
		mapped_memory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			CreateBuffer(vk_context, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, buffers[i], device_memory[i]);
			vkMapMemory(vk_context.m_device, device_memory[i], 0, size, 0, &mapped_memory[i]);
		}
	}

#pragma endregion

#pragma region Graphics Pipeline

	VkShaderModule CreateShaderModule(const VkContext& vk_context, const std::string& shader_file_name) {

		std::vector<char> shader_code = ReadFile(std::string(SHADERS_DIR) + shader_file_name);

		VkShaderModuleCreateInfo module_info{};
		module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_info.codeSize = shader_code.size();
		module_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

		VkShaderModule out;
		if (vkCreateShaderModule(vk_context.m_device, &module_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create shader module " + shader_file_name);
		}

		return out;
	}

	VkPipelineLayout CreatePipelineLayout(const VkContext& vk_context, const VkDescriptorSetLayout &descriptor_set_layout) {
		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		VkPipelineLayout pipeline_layout;
		if (vkCreatePipelineLayout(vk_context.m_device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Pipeline Layout");
		}

		return pipeline_layout;
	}

	VkPipeline CreateGraphicsPipeline(const VkContext& vk_context, const VkRenderPass &render_pass, const VkPipelineLayout &pipeline_layout, const std::string& vert_shader_name, const std::string& frag_shader_name) {

		VkShaderModule vert_module = CreateShaderModule(vk_context, vert_shader_name);
		VkShaderModule frag_module = CreateShaderModule(vk_context, frag_shader_name);

		VkPipelineShaderStageCreateInfo vert_shader_info{};
		vert_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_info.module = vert_module;
		vert_shader_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_info{};
		frag_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_info.module = frag_module;
		frag_shader_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stage_infos[] = { vert_shader_info, frag_shader_info };

		const std::vector<VkDynamicState> dynamic_state = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_info{};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.dynamicStateCount = dynamic_state.size();
		dynamic_state_info.pDynamicStates = dynamic_state.data();

		VkVertexInputBindingDescription binding_description = Vertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 2> attributes_descriptions = Vertex::GetVertexInputAttributeDescription();

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = attributes_descriptions.data();
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterization_info{};
		rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_info.depthClampEnable = VK_FALSE;
		rasterization_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_info.lineWidth = 1.0f;
		rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization_info.depthBiasEnable = VK_FALSE;
		rasterization_info.depthBiasConstantFactor = 0.0f;
		rasterization_info.depthBiasClamp = 0.0f;
		rasterization_info.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling_info{};
		multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling_info.sampleShadingEnable = VK_FALSE;
		multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling_info.minSampleShading = 1.0f;
		multisampling_info.pSampleMask = nullptr;
		multisampling_info.alphaToCoverageEnable = VK_FALSE;
		multisampling_info.alphaToOneEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewport_info{};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.scissorCount = 1;
		viewport_info.pScissors = nullptr;
		viewport_info.viewportCount = 1;
		viewport_info.pViewports = nullptr;

		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending_info{};
		color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending_info.logicOpEnable = VK_FALSE;
		color_blending_info.attachmentCount = 1;
		color_blending_info.pAttachments = &color_blend_attachment;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
		depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_info.depthTestEnable = VK_FALSE;
		depth_stencil_info.depthWriteEnable = VK_FALSE;
		depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_info.stencilTestEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
		graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphics_pipeline_info.stageCount = 2;
		graphics_pipeline_info.pStages = shader_stage_infos;
		graphics_pipeline_info.pVertexInputState = &vertex_input_info;
		graphics_pipeline_info.pInputAssemblyState = &input_assembly_info;
		graphics_pipeline_info.pRasterizationState = &rasterization_info;
		graphics_pipeline_info.pMultisampleState = &multisampling_info;
		graphics_pipeline_info.pViewportState = &viewport_info;
		graphics_pipeline_info.pDepthStencilState = &depth_stencil_info;
		graphics_pipeline_info.pColorBlendState = &color_blending_info;
		graphics_pipeline_info.pDynamicState = &dynamic_state_info;
		graphics_pipeline_info.layout = pipeline_layout;
		graphics_pipeline_info.renderPass = render_pass;
		graphics_pipeline_info.subpass = 0;
		graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		graphics_pipeline_info.basePipelineIndex = -1;

		VkPipeline out;
		if (vkCreateGraphicsPipelines(vk_context.m_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Pipeline");
		}

		vkDestroyShaderModule(vk_context.m_device, vert_module, nullptr);
		vkDestroyShaderModule(vk_context.m_device, frag_module, nullptr);

		return out;
	}

#pragma endregion

#pragma region Render Pass 

	VkRenderPass CreateRenderPass(const VkContext& vk_context) {

		VkAttachmentDescription	color_attachment{};
		color_attachment.format = vk_context.m_swap_chain_image_format.format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 1> attachments = { color_attachment };
		VkRenderPassCreateInfo render_pass_create_info{};
		render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_create_info.pAttachments = attachments.data();
		render_pass_create_info.subpassCount = 1;
		render_pass_create_info.pSubpasses = &subpass;
		render_pass_create_info.dependencyCount = 1;
		render_pass_create_info.pDependencies = &dependency;

		VkRenderPass render_pass;
		if (vkCreateRenderPass(vk_context.m_device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Render Pass");
		}

		return render_pass;
	}

#pragma endregion 

#pragma region Descriptors

	VkDescriptorSetLayout CreateDescriptorSetLayout(const VkContext& vk_context) {
		VkDescriptorSetLayoutBinding ubo_layout_binding{};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 1> bindings = { ubo_layout_binding };
		VkDescriptorSetLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = bindings.size();
		layout_info.pBindings = bindings.data();

		VkDescriptorSetLayout out;
		if (vkCreateDescriptorSetLayout(vk_context.m_device, &layout_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Descriptor Set Layout");
		}

		return out;
	}

	VkDescriptorPool CreateDescriptorPool(const VkContext &vk_context) {

		std::array<VkDescriptorPoolSize, 1> pool_sizes{};
		pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorPoolCreateInfo  pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPool descriptor_pool;
		if (vkCreateDescriptorPool(vk_context.m_device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Descriptor Pool");
		}

		return descriptor_pool;
	}

	std::vector<VkDescriptorSet> CreateDescriptorSets(const VkContext &vk_context, const std::vector<VkBuffer>& uniform_buffers, const VkDescriptorPool &descriptor_pool, const VkDescriptorSetLayout &descriptor_set_layout) {

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);
		VkDescriptorSetAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool;
		alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		alloc_info.pSetLayouts = descriptor_set_layouts.data();

		std::vector<VkDescriptorSet> descriptor_sets(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(vk_context.m_device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to allocate Descriptor Sets");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorBufferInfo buffer_info{};
			buffer_info.buffer = uniform_buffers[i];
			buffer_info.offset = 0;
			buffer_info.range = sizeof(UniformBufferObject);

			std::array<VkWriteDescriptorSet, 1> descriptor_writes{};
			descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[0].dstSet = descriptor_sets[i];
			descriptor_writes[0].dstBinding = 0;
			descriptor_writes[0].dstArrayElement = 0;
			descriptor_writes[0].descriptorCount = 1;
			descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes[0].pBufferInfo = &buffer_info;

			vkUpdateDescriptorSets(vk_context.m_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
		}

		return descriptor_sets;
	}

#pragma endregion

#pragma region Synchronisation

#pragma endregion

}

#pragma region Constructors

Renderer *CreateRenderer(const VkContext& vk_context) {

	std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	Renderer *out = new Renderer();

	CreateUniformBuffer(vk_context, out->m_uniform_buffers, out->m_uniform_buffers_memory, out->m_uniform_buffers_mapped_memory);

	out->m_render_pass = CreateRenderPass(vk_context);
	out->m_descriptor_set_layout = CreateDescriptorSetLayout(vk_context);
	out->m_descriptor_pool = CreateDescriptorPool(vk_context);
	out->m_pipeline_layout = CreatePipelineLayout(vk_context, out->m_descriptor_set_layout);
	out->m_graphics_pipeline = CreateGraphicsPipeline(vk_context, out->m_render_pass, out->m_pipeline_layout, "simple_shader_vert.spv", "simple_shader_frag.spv");
	out->m_descriptor_sets = CreateDescriptorSets(vk_context, out->m_uniform_buffers, out->m_descriptor_pool, out->m_descriptor_set_layout);
	out->m_graphics_command_pool = CreateCommandPool(vk_context, vk_context.m_indices.graphics_family.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	out->m_transfer_command_pool = CreateCommandPool(vk_context, vk_context.m_indices.transfer_family.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	out->m_graphics_command_buffer = CreateCommandBuffer(vk_context, out->m_graphics_command_pool, MAX_FRAMES_IN_FLIGHT);
	CreatePrimitiveBuffer<Vertex>(vk_context, out->m_transfer_command_pool, vertices, out->m_vertex_buffers, out->m_vertex_buffers_memory, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	out->m_image_available_semaphore = CreateSemaphores(vk_context, MAX_FRAMES_IN_FLIGHT);
	out->m_in_flight_fence = CreateFences(vk_context, MAX_FRAMES_IN_FLIGHT, VK_FENCE_CREATE_SIGNALED_BIT);

	return out;
}

void DestroyRenderer(const VkContext& vk_context, Renderer *renderer) {

	DestroySemaphores(vk_context, renderer->m_image_available_semaphore);
	DestroyFences(vk_context, renderer->m_in_flight_fence);

	vkDestroyBuffer(vk_context.m_device, renderer->m_vertex_buffers, nullptr);
	vkFreeMemory(vk_context.m_device, renderer->m_vertex_buffers_memory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroyBuffer(vk_context.m_device, renderer->m_uniform_buffers[i], nullptr);
		vkFreeMemory(vk_context.m_device, renderer->m_uniform_buffers_memory[i], nullptr);
	}

	vkDestroyCommandPool(vk_context.m_device, renderer->m_graphics_command_pool, nullptr);
	vkDestroyCommandPool(vk_context.m_device, renderer->m_transfer_command_pool, nullptr);
	
	vkDestroyDescriptorPool(vk_context.m_device, renderer->m_descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(vk_context.m_device, renderer->m_descriptor_set_layout, nullptr);

	vkDestroyPipeline(vk_context.m_device, renderer->m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(vk_context.m_device, renderer->m_pipeline_layout, nullptr);
	vkDestroyRenderPass(vk_context.m_device, renderer->m_render_pass, nullptr);
}

#pragma endregion