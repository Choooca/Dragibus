#pragma once

#include <vector>
#include <memory>

#include <glfw/glfw3.h>

#include <utils/custom_type.h>

struct RenderContext;

namespace Vulkan {

	class Renderer;
	class SwapChain;
	class ImageView;
	class Image;
	class DeviceMemory;
	class Framebuffer;
	class CommandPool;

	class SwapChainResources {

	public:
		SwapChainResources(Renderer* renderer);
		~SwapChainResources();

		int GetSwapChainImageCount();
		VkExtent2D GetSwapchainExtent();

		bool m_frame_buffer_resized = false;

		VkSwapchainKHR GetSwapchain();
		VkFramebuffer GetFramebuffer(int swap_chain_index);

	private:

		Renderer* m_renderer;

		VkPresentModeKHR m_present_mode;
		VkExtent2D m_extent;

		std::unique_ptr<SwapChain> m_swap_chain;
		std::vector<VkImage> m_images;
		std::vector<ImageView> m_image_views;
		std::unique_ptr<Image> m_depth_image;
		std::unique_ptr<DeviceMemory> m_depth_image_memory;
		std::unique_ptr<ImageView> m_depth_image_view;
		std::vector<Framebuffer> m_framebuffers;

		int m_swap_chain_image_count;

		VkPresentModeKHR ChooseSwapChainPresentMode(const SwapChainSupportDetails& swap_chain_support_details);
		VkExtent2D GetSwapChainExtent(const SwapChainSupportDetails& swap_chain_support_details);

		std::vector<VkImage> RetrieveSwapChainImage(const VkSwapchainKHR swap_chain);

		std::vector<ImageView> CreateSwapChainImageViews(const std::vector<VkImage> swap_chain_images, const VkImageAspectFlags aspect_flags);
	};
}