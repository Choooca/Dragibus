#include "pipeline_layout.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::PipelineLayout::PipelineLayout(Renderer* renderer)
	: m_renderer(renderer)
{
	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;

	VkDescriptorSetLayout descriptor_set_layout = m_renderer->GetDescriptorSetLayout();
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_renderer->GetDevice(), &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Pipeline Layout");
	}

}

Vulkan::PipelineLayout::~PipelineLayout()
{
	vkDestroyPipelineLayout(m_renderer->GetDevice(), m_pipeline_layout, nullptr);
}

VkPipelineLayout Vulkan::PipelineLayout::Get()
{
	return m_pipeline_layout;
}
