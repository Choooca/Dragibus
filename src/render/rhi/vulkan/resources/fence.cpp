#include "fence.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Fence::Fence(Renderer* renderer, const VkFenceCreateFlags& flags)
	: _renderer(renderer)
{
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = flags;

	if (vkCreateFence(_renderer->GetDevice(), &fence_info, nullptr, &_fence) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Fence.");
	}
}

Vulkan::Fence::~Fence()
{
	vkDestroyFence(_renderer->GetDevice(), _fence, nullptr);
}

VkFence Vulkan::Fence::Get()
{
	return _fence;
}
