#include "framebuffer.h"

#include <array>

#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::Framebuffer::Framebuffer(Renderer* renderer, const VkImageView& color_image_view, const VkImageView& depth_image_view, const VkExtent2D extent)
	: _renderer(renderer)
{
	std::array<VkImageView, 2> attachments{
		color_image_view,
		depth_image_view
	};

	VkFramebufferCreateInfo framebuffer_info{};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.renderPass = _renderer->GetRenderPass();
	framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebuffer_info.pAttachments = attachments.data();
	framebuffer_info.width = extent.width;
	framebuffer_info.height = extent.height;
	framebuffer_info.layers = 1;

	if (vkCreateFramebuffer(_renderer->GetDevice(), &framebuffer_info, nullptr, &_framebuffer) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Framebuffer.");
	}
}

Vulkan::Framebuffer::~Framebuffer()
{
	vkDestroyFramebuffer(_renderer->GetDevice(), _framebuffer, nullptr);
}

VkFramebuffer Vulkan::Framebuffer::Get()
{
	return _framebuffer;
}
