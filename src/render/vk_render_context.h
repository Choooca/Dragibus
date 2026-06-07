#pragma once

#include <vector>

#include <glfw/glfw3.h>

struct VkContext;

struct VkRenderContext {

	VkContext *m_vk_context;

	VkSurfaceFormatKHR m_swap_chain_image_format;
	VkPresentModeKHR m_swap_chain_present_mode;
	VkExtent2D m_swap_chain_extent;
	VkSwapchainKHR m_swap_chain;
	std::vector<VkImage> m_swap_chain_images;
	std::vector<VkImageView> m_swap_chain_image_views;

	VkRenderPass m_render_pass;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_graphics_pipeline;

	std::vector<VkFramebuffer> m_framebuffer;

};

VkRenderContext CreateRenderContext(VkContext &ctx);
void DestroyRenderContext(const VkRenderContext& render_ctx);