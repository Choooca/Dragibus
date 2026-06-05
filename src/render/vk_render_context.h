#pragma once

#include <vector>

#include <glfw/glfw3.h>

struct VkRenderContext {
	VkSurfaceFormatKHR m_swap_chain_image_format;
	VkPresentModeKHR m_swap_chain_present_mode;
	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
};

struct VkContext;

VkRenderContext CreateRenderContext(const VkContext &ctx);
void DestroyRenderContext(const VkContext ctx, const VkRenderContext& render_ctx);