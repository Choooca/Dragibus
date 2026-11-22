#include "pipeline_utils.h"
#include <utils/build_macro.h>
#include <utils/files.h>
#include <render/rhi/shader_constants.h>
#include <render/rhi/vulkan/context.h>

vk::raii::Pipeline Vulkan::CreateGraphicsPipeline(const Vulkan::Context& context, const Vulkan::GraphicsPipelineDesc& desc)
{
	auto shader_code = ReadFile(std::string(SHADERS_DIR) + desc.file_name);
	vk::ShaderModuleCreateInfo shader_module_create_info{ .codeSize = shader_code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(shader_code.data()) };
	vk::raii::ShaderModule shader_module = vk::raii::ShaderModule(context.GetDevice(), shader_module_create_info);

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

	std::vector<vk::DynamicState> dynamic_state = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamic_state_info{ .dynamicStateCount = static_cast<uint32_t>(dynamic_state.size()), .pDynamicStates = dynamic_state.data() };

	vk::PipelineVertexInputStateCreateInfo vertex_input_info = {
		.vertexBindingDescriptionCount = static_cast<uint32_t>(desc.vertex_input_bindings.size()),
		.pVertexBindingDescriptions = desc.vertex_input_bindings.data(),
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(desc.vertex_input_attributes.size()),
		.pVertexAttributeDescriptions = desc.vertex_input_attributes.data()
	};
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{ .topology = vk::PrimitiveTopology::eTriangleList };
	vk::PipelineViewportStateCreateInfo viewport_state_info{ .viewportCount = 1, .scissorCount = 1 };
	vk::PipelineRasterizationStateCreateInfo rasterizer_info{
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = desc.cull_mode,
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
		.layout = desc.pipeline_layout,
		.renderPass = nullptr },
	{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &(desc.attachment_format), .depthAttachmentFormat = desc.depth_format } };

	return vk::raii::Pipeline(context.GetDevice(), nullptr, pipeline_info_chain.get<vk::GraphicsPipelineCreateInfo>());
}
