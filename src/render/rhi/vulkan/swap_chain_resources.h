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

		bool _frame_buffer_resized = false;

		VkSwapchainKHR GetSwapchain();
		VkFramebuffer GetFramebuffer(int swap_chain_index);

	private:

		Renderer* _renderer;

		VkPresentModeKHR _present_mode;
		VkExtent2D _extent;

		std::unique_ptr<SwapChain> _swap_chain;
		std::vector<VkImage> _images;
		std::vector<ImageView> _image_views;
		std::unique_ptr<Image> _depth_image;
		std::unique_ptr<DeviceMemory> _depth_image_memory;
		std::unique_ptr<ImageView> _depth_image_view;
		std::vector<Framebuffer> _framebuffers;

		int _swap_chain_image_count;

		VkPresentModeKHR ChooseSwapChainPresentMode(const SwapChainSupportDetails& swap_chain_support_details);
		VkExtent2D GetSwapChainExtent(const SwapChainSupportDetails& swap_chain_support_details);

		std::vector<VkImage> RetrieveSwapChainImage(const VkSwapchainKHR swap_chain);

		std::vector<ImageView> CreateSwapChainImageViews(const std::vector<VkImage> swap_chain_images, const VkImageAspectFlags aspect_flags);
	};
}