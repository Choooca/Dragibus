#include "semaphore.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Semaphore::Semaphore(Renderer* renderer, const VkSemaphoreCreateFlags flags)
	: _renderer(renderer)
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.flags = flags;

	if (vkCreateSemaphore(_renderer->GetDevice(), &semaphore_info, nullptr, &_semaphore) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Semaphore.");
	}
}

Vulkan::Semaphore::~Semaphore()
{
	vkDestroySemaphore(_renderer->GetDevice(), _semaphore, nullptr);
}

VkSemaphore Vulkan::Semaphore::Get()
{
	return _semaphore;
}
