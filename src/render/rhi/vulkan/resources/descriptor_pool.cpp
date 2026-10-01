#include "descriptor_pool.h"

#include <utils/debug_macro.h>
#include <utils/custom_type.h>
#include <render/rhi/vulkan/resources/uniform_buffer.h>
#include <render/rhi/vulkan/renderer.h>
#include <render/primitives.h>

Vulkan::DescriptorPool::DescriptorPool(Renderer* renderer, const int descriptor_count)
	: _renderer(renderer)
{
	std::array<VkDescriptorPoolSize, 2> pool_sizes{};
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(descriptor_count);
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(descriptor_count);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo  pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(descriptor_count);

	if (vkCreateDescriptorPool(_renderer->GetDevice(), &pool_info, nullptr, &_descriptor_pool) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Descriptor Pool");
	}
}

Vulkan::DescriptorPool::~DescriptorPool()
{
	vkDestroyDescriptorPool(_renderer->GetDevice(), _descriptor_pool, nullptr);
}

VkDescriptorPool Vulkan::DescriptorPool::Get()
{
	return _descriptor_pool;
}

void Vulkan::DescriptorPool::CreateDescriptorSet(const std::vector<UniformBuffer> &uniform_buffers, const VkImageView &image_view, const VkSampler &sampler, int32_t count)
{
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts(count, _renderer->GetDescriptorSetLayout());
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = _descriptor_pool;
	alloc_info.descriptorSetCount = count;
	alloc_info.pSetLayouts = descriptor_set_layouts.data();

	_descriptor_sets.resize(count);
	if (vkAllocateDescriptorSets(_renderer->GetDevice(), &alloc_info, _descriptor_sets.data()) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to allocate Descriptor Sets");
	}

	for (size_t i = 0; i < count; ++i) {
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = uniform_buffers[i].GetBuffer();
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = image_view;
		image_info.sampler = sampler;

		std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = _descriptor_sets[i];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = _descriptor_sets[i];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].pImageInfo = &image_info;

		vkUpdateDescriptorSets(_renderer->GetDevice(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
	}
}

VkDescriptorSet Vulkan::DescriptorPool::GetDescriptorSet(int index) { return _descriptor_sets[index]; }
