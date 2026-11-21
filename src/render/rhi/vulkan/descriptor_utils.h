#pragma once

#include <vector>
#include <span>
#include <variant>
#include <array>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
	class Context;

	using DescriptorBindingResource = std::variant<std::vector<vk::DescriptorImageInfo>, std::vector<vk::DescriptorBufferInfo>>;

	struct DescriptorSetGroupDesc {
		vk::DescriptorSetLayout descriptor_set_layout;

		std::span<const vk::DescriptorSetLayoutBinding> bindings;
		std::vector<std::vector<DescriptorBindingResource>> resources;
	};

	struct DescriptorSetGroup {
		vk::raii::DescriptorPool pool = nullptr;
		std::vector<vk::raii::DescriptorSet> sets;
	};

	DescriptorSetGroup CreateDescriptorSetGroup(const Vulkan::Context& context, const DescriptorSetGroupDesc& desc);
}

