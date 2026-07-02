#include "image_view.h"
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::ImageView::ImageView(Renderer* renderer, const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspect_flags)
	: m_renderer(renderer)
{
	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.image = image;
	image_view_create_info.format = format;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

	image_view_create_info.subresourceRange.aspectMask = aspect_flags;
	image_view_create_info.subresourceRange.layerCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseMipLevel = 0;

	if (vkCreateImageView(m_renderer->GetDevice(), &image_view_create_info, nullptr, &m_image_view) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create Image View");
	}
}

Vulkan::ImageView::~ImageView()
{
	vkDestroyImageView(m_renderer->GetDevice(), m_image_view, nullptr);
}

VkImageView Vulkan::ImageView::Get()
{
	return m_image_view;
}
