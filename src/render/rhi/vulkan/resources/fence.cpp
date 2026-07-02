#include "fence.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Fence::Fence(Renderer* renderer, const VkFenceCreateFlags& flags)
	: m_renderer(renderer)
{
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = flags;

	if (vkCreateFence(m_renderer->GetDevice(), &fence_info, nullptr, &m_fence) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Fence.");
	}
}

Vulkan::Fence::~Fence()
{
	vkDestroyFence(m_renderer->GetDevice(), m_fence, nullptr);
}

VkFence Vulkan::Fence::Get()
{
	return m_fence;
}
