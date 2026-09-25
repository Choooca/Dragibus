#include "graphics_pipeline.h"

#include <render/rhi/vulkan/handles/pipeline_layout.h>
#include <render/rhi/vulkan/resources/shader_module.h>
#include <render/primitives.h>
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::GraphicsPipeline::GraphicsPipeline(Renderer* renderer, const std::string& vert_shader_name, const std::string& frag_shader_name)
	: m_renderer(renderer)
{
	m_pipeline_layout = std::make_unique<PipelineLayout>(m_renderer);

	ShaderModule vert_module = ShaderModule(m_renderer, vert_shader_name);
	ShaderModule frag_module = ShaderModule(m_renderer, frag_shader_name);

	VkPipelineShaderStageCreateInfo vert_shader_info{};
	vert_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_info.module = vert_module.Get();
	vert_shader_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_info{};
	frag_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_info.module = frag_module.Get();
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
	std::array<VkVertexInputAttributeDescription, 4> attributes_descriptions = Vertex::GetVertexInputAttributeDescription();

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
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
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
	graphics_pipeline_info.layout = m_pipeline_layout->Get();
	graphics_pipeline_info.renderPass = m_renderer->GetRenderPass();
	graphics_pipeline_info.subpass = 0;
	graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_info.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_renderer->GetDevice(), VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &m_pipeline) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Pipeline");
	}
}

Vulkan::GraphicsPipeline::~GraphicsPipeline()
{
	vkDestroyPipeline(m_renderer->GetDevice(), m_pipeline, nullptr);
}

VkPipeline Vulkan::GraphicsPipeline::Get()
{
	return m_pipeline;
}

VkPipelineLayout Vulkan::GraphicsPipeline::GetPipelineLayout() { return m_pipeline_layout->Get(); }
