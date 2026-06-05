#include "vk_render_context.h"

#include <algorithm>

#include <render/vk_utils.h>
#include <utils/debug_macro.h>

namespace {

#pragma region Resources

	VkImageView CreateImageView(const VkContext& ctx, const VkImage& image, const VkFormat format, VkImageAspectFlags aspect_flags) {

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

		VkImageView out;
		if (vkCreateImageView(ctx.m_device, &image_view_create_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Image View");
		}

		return out;
	}

#pragma endregion

#pragma region Swap Chain

	VkSurfaceFormatKHR ChooseSwapChainImageFormat(const SwapChainSupportDetails &swap_chain_support_details) {

		const std::vector<VkSurfaceFormatKHR>& available_formats = swap_chain_support_details.formats;

		VkSurfaceFormatKHR image_format = available_formats[0];
		for (size_t i = 1; i < available_formats.size(); ++i) {
			const VkSurfaceFormatKHR& available_format = available_formats[i];
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				image_format = available_format;
				break;
			}
		}

		return image_format;
	}

	VkPresentModeKHR ChooseSwapChainPresentMode(const SwapChainSupportDetails& swap_chain_support_details) {

		const std::vector<VkPresentModeKHR>& available_present_modes = swap_chain_support_details.present_modes;

		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for (const VkPresentModeKHR& available_present_mode : available_present_modes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				present_mode = available_present_mode;
			}
		}

		return present_mode;
	}

	VkSwapchainKHR CreateSwapChain(const VkRenderContext &render_ctx, const SwapChainSupportDetails& swap_chain_support_details) {

		VkExtent2D image_extent;
		const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			image_extent = capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(render_ctx.m_vk_context.m_window, &width, &height);

			image_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			image_extent.width = std::clamp(image_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			image_extent.height = std::clamp(image_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		uint32_t image_count = capabilities.minImageCount;
		if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < image_count) {
			image_count = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swap_chain_info{};
		swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_chain_info.surface = render_ctx.m_vk_context.m_surface;
		swap_chain_info.minImageCount = image_count;
		swap_chain_info.imageFormat = render_ctx.m_swap_chain_image_format.format;
		swap_chain_info.imageColorSpace = render_ctx.m_swap_chain_image_format.colorSpace;
		swap_chain_info.imageExtent = image_extent;
		swap_chain_info.imageArrayLayers = 1;
		swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swap_chain_info.presentMode = render_ctx.m_swap_chain_present_mode;

		uint32_t queue_family_indices[] = { render_ctx.m_vk_context.m_indices.graphics_family.value(), render_ctx.m_vk_context.m_indices.present_family.value()};

		if (render_ctx.m_vk_context.m_indices.graphics_family.value() != render_ctx.m_vk_context.m_indices.present_family.value()) {
			swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swap_chain_info.queueFamilyIndexCount = 2;
			swap_chain_info.pQueueFamilyIndices = queue_family_indices;
		}
		else {
			swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swap_chain_info.preTransform = capabilities.currentTransform;
		swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swap_chain_info.clipped = VK_TRUE;

		VkSwapchainKHR out;
		if (vkCreateSwapchainKHR(render_ctx.m_vk_context.m_device, &swap_chain_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create swap chain");
		}

		return out;
	}

	std::vector<VkImage> RetrieveSwapChainImage(const VkContext &ctx, const VkRenderContext &render_ctx) {
		
		uint32_t swap_chain_image_count; 
		vkGetSwapchainImagesKHR(ctx.m_device, render_ctx.m_swap_chain, &swap_chain_image_count, nullptr);

		std::vector<VkImage> swap_chain_images(swap_chain_image_count);
		vkGetSwapchainImagesKHR(ctx.m_device, render_ctx.m_swap_chain, &swap_chain_image_count, swap_chain_images.data());

		return swap_chain_images;

	}

	std::vector<VkImageView> CreateSwapChainImageViews(const VkRenderContext &render_context) {

		size_t swap_chain_image_count = render_context.m_swap_chain_images.size();
		std::vector<VkImageView> swap_chain_image_views(swap_chain_image_count);

		for (size_t i = 0; i < swap_chain_image_count; ++i) {
			swap_chain_image_views[i] = CreateImageView(render_context.m_vk_context, render_context.m_swap_chain_images[i], render_context.m_swap_chain_image_format.format, VK_IMAGE_ASPECT_COLOR_BIT);
		}

		return swap_chain_image_views;
	}

#pragma endregion

#pragma region Destruction

	void DestroySwapChain(const VkRenderContext &render_ctx, const VkSwapchainKHR &swap_chain) {
		for (const VkImageView& image_view : render_ctx.m_swap_chain_image_views) {
			vkDestroyImageView(render_ctx.m_vk_context.m_device, image_view, nullptr);
		}

		vkDestroySwapchainKHR(render_ctx.m_vk_context.m_device, swap_chain, nullptr);
	}

#pragma endregion

}

#pragma region Render Context

VkRenderContext CreateRenderContext(const VkContext& ctx)
{
	VkRenderContext out;

	out.m_vk_context = ctx;

	SwapChainSupportDetails swap_chain_support_details = GetSwapChainSupportDetails(out.m_vk_context.m_physical_device, out.m_vk_context.m_surface);

	out.m_swap_chain_image_format = ChooseSwapChainImageFormat(swap_chain_support_details);
	out.m_swap_chain_present_mode = ChooseSwapChainPresentMode(swap_chain_support_details);
	out.m_swap_chain = CreateSwapChain(out, swap_chain_support_details);
	out.m_swap_chain_images = RetrieveSwapChainImage(ctx, out);
	out.m_swap_chain_image_views = CreateSwapChainImageViews(out);

	return out;
}

void DestroyRenderContext(const VkContext ctx, const VkRenderContext& render_ctx)
{
	DestroySwapChain(render_ctx, render_ctx.m_swap_chain);
}

#pragma endregion