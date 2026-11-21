#include "descriptor_utils.h"

#include <render/rhi/vulkan/context.h>
#include <utils/debug_macro.h>

Vulkan::DescriptorSetGroup Vulkan::CreateDescriptorSetGroup(const Vulkan::Context& context, const Vulkan::DescriptorSetGroupDesc& desc)
{
	Vulkan::DescriptorSetGroup out{};

	uint32_t set_count = static_cast<uint32_t>(desc.resources.size());

	std::vector<vk::DescriptorPoolSize> pool_sizes;
	pool_sizes.reserve(desc.bindings.size());

	for (const auto& binding : desc.bindings) {
		pool_sizes.push_back({ .type = binding.descriptorType, .descriptorCount = binding.descriptorCount * set_count});
	}

	vk::DescriptorPoolCreateInfo descriptor_pool_info = {
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = set_count,
		.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
		.pPoolSizes = pool_sizes.data()
	};

	out.pool = vk::raii::DescriptorPool(context.GetDevice(), descriptor_pool_info);

	std::vector<vk::DescriptorSetLayout> layouts(set_count, desc.descriptor_set_layout);
	vk::DescriptorSetAllocateInfo alloc_info = {
		.descriptorPool = out.pool,
		.descriptorSetCount = set_count,
		.pSetLayouts = layouts.data()
	};

	out.sets = context.GetDevice().allocateDescriptorSets(alloc_info);

	std::vector<vk::WriteDescriptorSet> write_descriptor_sets;

	for (uint32_t set = 0; set < desc.resources.size(); ++set) {

		if (desc.bindings.size() != desc.resources[set].size()) {
			THROW_RUNTIME_ERROR("Bindings numbers differ between layout and resource");
		}

		for (uint32_t binding = 0; binding < desc.resources[set].size(); ++binding) {
			const auto& resource = desc.resources[set][binding];

			const auto *image = std::get_if<std::vector<vk::DescriptorImageInfo>>(&resource);
			const auto *buffer = std::get_if<std::vector<vk::DescriptorBufferInfo>>(&resource);

			if ((image ? image->size() : buffer->size()) != desc.bindings[binding].descriptorCount) {
				THROW_RUNTIME_ERROR("Descriptor count differ between layout and resource");
			}

			vk::WriteDescriptorSet write_descriptor_set = {
					.dstSet = out.sets[set],
					.dstBinding = desc.bindings[binding].binding,
					.dstArrayElement = 0,
					.descriptorCount = desc.bindings[binding].descriptorCount,
					.descriptorType = desc.bindings[binding].descriptorType,
					.pImageInfo = image ? image->data() : nullptr,
					.pBufferInfo = buffer ? buffer->data() : nullptr
			};
			write_descriptor_sets.push_back(write_descriptor_set);
		}
	}

	context.GetDevice().updateDescriptorSets(write_descriptor_sets, {});

	return out;
}