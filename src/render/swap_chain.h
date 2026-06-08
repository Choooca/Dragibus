#pragma once

#include <vector>

#include <glfw/glfw3.h>

struct VkContext;

struct SwapChain {

	VkPresentModeKHR m_swap_chain_present_mode;
	VkExtent2D m_swap_chain_extent;
	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
	std::vector<VkImageView> m_swap_chain_image_views;
	std::vector<VkFramebuffer> m_framebuffers;

	std::vector<VkSemaphore> m_render_finish_semaphore;
};

std::vector<VkFramebuffer> CreateFramebuffer(const VkContext& vk_context, const SwapChain& swap_chain, const VkRenderPass& render_pass);

SwapChain *CreateSwapChain(const VkContext &vk_context);
void DestroySwapChain(const VkContext& vk_context, SwapChain* swap_chain);