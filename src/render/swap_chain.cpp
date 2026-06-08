#include "swap_chain.h"

#include <algorithm>
#include <array>

#include <render/vk_context.h>
#include <render/vk_utils.h>
#include <utils/debug_macro.h>
#include <utils/files.h>
#include <utils/build_macro.h>
#include <render/primitives.h>
#include <render/vk_resources.h>

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

	VkExtent2D GetSwapChainExtent(const VkContext &vk_context, const SwapChainSupportDetails& swap_chain_support_details) {
		VkExtent2D image_extent;
		const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			image_extent = capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(vk_context.m_window, &width, &height);

			image_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			image_extent.width = std::clamp(image_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			image_extent.height = std::clamp(image_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		}

		return image_extent;
	}

	VkSwapchainKHR CreateSwapChainHandle(const VkContext &vk_context, const VkExtent2D &swap_chain_extents, const VkPresentModeKHR &swap_chain_present_mode, const SwapChainSupportDetails& swap_chain_support_details) {

		const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

		uint32_t image_count = capabilities.minImageCount;
		if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < image_count) {
			image_count = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swap_chain_info{};
		swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_chain_info.surface = vk_context.m_surface;
		swap_chain_info.minImageCount = image_count;
		swap_chain_info.imageFormat = vk_context.m_swap_chain_image_format.format;
		swap_chain_info.imageColorSpace = vk_context.m_swap_chain_image_format.colorSpace;
		swap_chain_info.imageExtent = swap_chain_extents;
		swap_chain_info.imageArrayLayers = 1;
		swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swap_chain_info.presentMode = swap_chain_present_mode;

		uint32_t queue_family_indices[] = { vk_context.m_indices.graphics_family.value(), vk_context.m_indices.present_family.value()};

		if (vk_context.m_indices.graphics_family.value() != vk_context.m_indices.present_family.value()) {
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
		if (vkCreateSwapchainKHR(vk_context.m_device, &swap_chain_info, nullptr, &out) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create swap chain");
		}

		return out;
	}

	std::vector<VkImage> RetrieveSwapChainImage(const VkContext &vk_context, const VkSwapchainKHR &swap_chain) {
		
		uint32_t swap_chain_image_count; 
		vkGetSwapchainImagesKHR(vk_context.m_device, swap_chain, &swap_chain_image_count, nullptr);

		std::vector<VkImage> swap_chain_images(swap_chain_image_count);
		vkGetSwapchainImagesKHR(vk_context.m_device, swap_chain, &swap_chain_image_count, swap_chain_images.data());

		return swap_chain_images;

	}

	std::vector<VkImageView> CreateSwapChainImageViews(const VkContext &vk_context, const std::vector<VkImage> swap_chain_images) {

		size_t swap_chain_image_count = swap_chain_images.size();
		std::vector<VkImageView> swap_chain_image_views(swap_chain_image_count);

		for (size_t i = 0; i < swap_chain_image_count; ++i) {
			swap_chain_image_views[i] = CreateImageView(vk_context, swap_chain_images[i], vk_context.m_swap_chain_image_format.format, VK_IMAGE_ASPECT_COLOR_BIT);
		}

		return swap_chain_image_views;
	}

	void CleanupSwapChain(const VkContext &vk_context,  SwapChain &swap_chain) {

		for (VkFramebuffer framebuffer : swap_chain.m_framebuffers) {
			vkDestroyFramebuffer(vk_context.m_device, framebuffer, nullptr);
		}

		for (const VkImageView& image_view : swap_chain.m_swap_chain_image_views) {
			vkDestroyImageView(vk_context.m_device, image_view, nullptr);
		}

		vkDestroySwapchainKHR(vk_context.m_device, swap_chain.m_swap_chain, nullptr);
	}

#pragma endregion

#pragma region Frambuffer

	void FramebufferResizedCallback(GLFWwindow* window, int width, int height) {
		SwapChain* swap_chain = reinterpret_cast<SwapChain*>(window);
		swap_chain->m_frame_buffer_resized = true;
	}

#pragma endregion

}

#pragma region Framebuffer

std::vector<VkFramebuffer> CreateFramebuffer(const VkContext& vk_context, const SwapChain& swap_chain, const VkRenderPass& render_pass) {

	size_t size = swap_chain.m_swap_chain_image_views.size();
	std::vector<VkFramebuffer> framebuffers(size);

	for (size_t i = 0; i < size; i++) {
		std::array<VkImageView, 1> attachments{
			swap_chain.m_swap_chain_image_views[i]
		};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = swap_chain.m_swap_chain_extent.width;
		framebuffer_info.height = swap_chain.m_swap_chain_extent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(vk_context.m_device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			THROW_RUNTIME_ERROR("Failed to create Framebuffer.");
		}
	}

	return framebuffers;
}

#pragma endregion

#pragma region Swap Chain

SwapChain *CreateSwapChain(const VkContext &vk_context)
{
	SwapChain *out = new SwapChain();

	glfwSetWindowUserPointer(vk_context.m_window, out);
	glfwSetFramebufferSizeCallback(vk_context.m_window, FramebufferResizedCallback);

	SwapChainSupportDetails swap_chain_support_details = GetSwapChainSupportDetails(vk_context.m_physical_device, vk_context.m_surface);

	out->m_swap_chain_present_mode = ChooseSwapChainPresentMode(swap_chain_support_details);
	out->m_swap_chain_extent = GetSwapChainExtent(vk_context, swap_chain_support_details);
	out->m_swap_chain = CreateSwapChainHandle(vk_context, out->m_swap_chain_extent, out->m_swap_chain_present_mode, swap_chain_support_details);
	out->m_swap_chain_images = RetrieveSwapChainImage(vk_context, out->m_swap_chain);
	out->m_swap_chain_image_views = CreateSwapChainImageViews(vk_context, out->m_swap_chain_images);
	out->m_render_finish_semaphore = CreateSemaphores(vk_context, out->m_swap_chain_images.size());

	return out;
}

void DestroySwapChain(const VkContext &vk_context, SwapChain *swap_chain) {

	DestroySemaphores(vk_context, swap_chain->m_render_finish_semaphore);
	CleanupSwapChain(vk_context, *swap_chain);

	delete swap_chain;
}

void RecreateSwapChain(const VkContext& vk_context, SwapChain &swap_chain, const VkRenderPass &render_pass) {

	int width = 0, height = 0;
	glfwGetFramebufferSize(vk_context.m_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(vk_context.m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vk_context.m_device);

	CleanupSwapChain(vk_context, swap_chain);

	SwapChainSupportDetails swap_chain_support_details = GetSwapChainSupportDetails(vk_context.m_physical_device, vk_context.m_surface);
	swap_chain.m_swap_chain_extent = GetSwapChainExtent(vk_context, swap_chain_support_details);
	swap_chain.m_swap_chain = CreateSwapChainHandle(vk_context, swap_chain.m_swap_chain_extent, swap_chain.m_swap_chain_present_mode, swap_chain_support_details);
	swap_chain.m_swap_chain_images = RetrieveSwapChainImage(vk_context, swap_chain.m_swap_chain);
	swap_chain.m_swap_chain_image_views = CreateSwapChainImageViews(vk_context, swap_chain.m_swap_chain_images);
	swap_chain.m_framebuffers = CreateFramebuffer(vk_context, swap_chain, render_pass);
}

#pragma endregion