#pragma once

#include <glfw/glfw3.h>

struct VkRenderContext {
	VkSwapchainKHR m_swap_chain;
};

struct VkContext;

VkRenderContext CreateRenderContext(const VkContext &ctx);
void DestroyRenderContext(const VkContext ctx, const VkRenderContext& render_ctx);