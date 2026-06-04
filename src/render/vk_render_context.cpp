#include "vk_render_context.h"

#include <algorithm>

#include <render/vk_context.h>
#include <render/vk_utils.h>
#include <utils/debug_macro.h>

namespace {

#pragma region Initialization

	VkSwapchainKHR CreateSwapChain(const VkContext& ctx) {
		SwapChainSupportDetails swap_chain_support_details = GetSwapChainSupportDetails(ctx.m_physical_device, ctx.m_surface);

		const std::vector<VkSurfaceFormatKHR> &available_formats = swap_chain_support_details.formats;

		VkSurfaceFormatKHR image_format = available_formats[0];
		for (size_t i = 1; i < available_formats.size(); ++i) {
			const VkSurfaceFormatKHR& available_format = available_formats[i];
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				image_format = available_format;
				break;
			}
		}

		const std::vector<VkPresentModeKHR>& available_present_modes = swap_chain_support_details.present_modes;

		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for (const VkPresentModeKHR& available_present_mode : available_present_modes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				present_mode = available_present_mode;
			}
		}

		VkExtent2D image_extent;
		const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			image_extent = capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(ctx.m_window, &width, &height);

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
		swap_chain_info.surface = ctx.m_surface;
		swap_chain_info.minImageCount = image_count;
		swap_chain_info.imageFormat = image_format.format;
		swap_chain_info.imageColorSpace = image_format.colorSpace;
		swap_chain_info.imageExtent = image_extent;
		swap_chain_info.imageArrayLayers = 1;
		swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swap_chain_info.presentMode = present_mode;

		uint32_t queue_family_indices[] = {ctx.m_indices.graphics_family.value(), ctx.m_indices.present_family.value()};

		if (ctx.m_indices.graphics_family.value() != ctx.m_indices.present_family.value()) {
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
		if (vkCreateSwapchainKHR(ctx.m_device, &swap_chain_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create swap chain");
		}

		return out;
	}

#pragma endregion

#pragma region Destruction

	void DestroySwapChain(const VkContext &ctx, const VkSwapchainKHR &swap_chain) {
		vkDestroySwapchainKHR(ctx.m_device, swap_chain, nullptr);
	}

#pragma endregion

}

#pragma region Render Context

VkRenderContext CreateRenderContext(const VkContext& ctx)
{
	VkRenderContext out;

	out.m_swap_chain = CreateSwapChain(ctx);

	return out;
}

void DestroyRenderContext(const VkContext ctx, const VkRenderContext& render_ctx)
{
	DestroySwapChain(ctx, render_ctx.m_swap_chain);
}

#pragma endregion