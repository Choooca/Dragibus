#pragma once

#include <vector>

#include <glfw/glfw3.h>

struct VkContext;
struct Renderer;

struct SwapChain {

	VkPresentModeKHR m_swap_chain_present_mode;
	VkExtent2D m_swap_chain_extent;
	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
	std::vector<VkImageView> m_swap_chain_image_views;
	std::vector<VkFramebuffer> m_framebuffers;

	std::vector<VkSemaphore> m_render_finish_semaphore;

	VkImage m_depth_image;
	VkDeviceMemory m_depth_image_memory;
	VkImageView m_depth_image_view;

	bool m_frame_buffer_resized;
};

std::vector<VkFramebuffer> CreateFramebuffer(const VkContext& vk_context, const SwapChain& swap_chain, const VkImageView& depth_image_view, const VkRenderPass& render_pass);

SwapChain *CreateSwapChain(const VkContext &vk_context);
void DestroySwapChain(const VkContext& vk_context, SwapChain* swap_chain);
void RecreateSwapChain(const VkContext& vk_context, const Renderer& renderer, SwapChain& swap_chain);

void CreateDepthResources(const VkContext& vk_context, const Renderer& renderer, const VkExtent2D extent, VkImage& image, VkImageView& image_view, VkDeviceMemory& image_memory);