#include "framebuffer.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Framebuffer::Framebuffer(Renderer* renderer, const VkImageView& color_image_view, const VkImageView& depth_image_view, const VkExtent2D extent)
	: m_renderer(renderer)
{
	std::array<VkImageView, 2> attachments{
		color_image_view,
		depth_image_view
	};

	VkFramebufferCreateInfo framebuffer_info{};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.renderPass = m_renderer->GetRenderPass();
	framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebuffer_info.pAttachments = attachments.data();
	framebuffer_info.width = extent.width;
	framebuffer_info.height = extent.height;
	framebuffer_info.layers = 1;

	if (vkCreateFramebuffer(m_renderer->GetDevice(), &framebuffer_info, nullptr, &m_framebuffer) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Framebuffer.");
	}
}

Vulkan::Framebuffer::~Framebuffer()
{
	vkDestroyFramebuffer(m_renderer->GetDevice(), m_framebuffer, nullptr);
}

VkFramebuffer Vulkan::Framebuffer::Get()
{
	return m_framebuffer;
}
