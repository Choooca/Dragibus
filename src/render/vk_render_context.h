#pragma once

#include <vector>

#include <glfw/glfw3.h>

struct VkContext;

struct VkRenderContext {

	VkContext m_vk_context;

	VkSurfaceFormatKHR m_swap_chain_image_format;
	VkPresentModeKHR m_swap_chain_present_mode;
	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
	std::vector<VkImageView> m_swap_chain_image_views;
};

VkRenderContext CreateRenderContext(const VkContext &ctx);
void DestroyRenderContext(const VkContext ctx, const VkRenderContext& render_ctx);