#include "descriptor_set_layout.h"

#include <array>

#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::DescriptorSetLayout::DescriptorSetLayout(Renderer* renderer)
	: m_renderer(renderer)
{
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding image_layout_binding{};
	image_layout_binding.binding = 1;
	image_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_layout_binding.descriptorCount = 1;
	image_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	image_layout_binding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, image_layout_binding };
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = bindings.size();
	layout_info.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_renderer->GetDevice(), &layout_info, nullptr, &m_descriptor_set_layout) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Descriptor Set Layout");
	}
}

Vulkan::DescriptorSetLayout::~DescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_renderer->GetDevice(), m_descriptor_set_layout, nullptr);
}

VkDescriptorSetLayout Vulkan::DescriptorSetLayout::Get()
{
	return m_descriptor_set_layout;
}
