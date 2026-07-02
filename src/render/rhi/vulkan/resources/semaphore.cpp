#include "semaphore.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Semaphore::Semaphore(Renderer* renderer, const VkSemaphoreCreateFlags flags)
	: m_renderer(renderer)
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.flags = flags;

	if (vkCreateSemaphore(m_renderer->GetDevice(), &semaphore_info, nullptr, &m_semaphore) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Semaphore.");
	}
}

Vulkan::Semaphore::~Semaphore()
{
	vkDestroySemaphore(m_renderer->GetDevice(), m_semaphore, nullptr);
}

VkSemaphore Vulkan::Semaphore::Get()
{
	return m_semaphore;
}
