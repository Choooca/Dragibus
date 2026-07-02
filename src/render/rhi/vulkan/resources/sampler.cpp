#include "sampler.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Sampler::Sampler(Renderer* renderer)
	: m_renderer(renderer)
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_renderer->GetPhysicalDevice(), &properties);

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	if (vkCreateSampler(m_renderer->GetDevice(), &sampler_info, nullptr, &m_sampler) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create sampler");
	}
}

Vulkan::Sampler::~Sampler()
{
	vkDestroySampler(m_renderer->GetDevice(), m_sampler, nullptr);
}

VkSampler Vulkan::Sampler::Get()
{
	return m_sampler;
}
