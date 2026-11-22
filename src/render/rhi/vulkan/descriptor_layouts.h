#pragma once

#include <array>
#include <vulkan/vulkan.hpp>
#include <render/rhi/vulkan/model.h>

namespace Vulkan {

	constexpr std::array<vk::DescriptorSetLayoutBinding, 1> SCN_BINDINGS{ {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment }
	} };

	constexpr std::array<vk::DescriptorSetLayoutBinding, 1> OBJ_BINDINGS{ {
		{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex }
	} };

	constexpr std::array<vk::DescriptorSetLayoutBinding, 2> MAT_BINDINGS{ {
		{ 0, vk::DescriptorType::eCombinedImageSampler, Vulkan::Material::MATERIAL_TEXTURE_COUNT, vk::ShaderStageFlagBits::eFragment },
		{ 1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment }
	} };

	constexpr std::array<vk::DescriptorSetLayoutBinding, 1> TONEMAPPING_BINDINGS{ {
		{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
	} };
}