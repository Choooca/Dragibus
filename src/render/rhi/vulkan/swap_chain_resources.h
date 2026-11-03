#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {

	class Renderer;

	class SwapChainResources {

	public:
		SwapChainResources(Renderer* renderer);
		~SwapChainResources();

		int GetSwapChainImageCount();
		vk::Extent2D &GetSwapChainExtent();

		bool _frame_buffer_resized = false;

		vk::raii::SwapchainKHR &GetSwapchain();

		vk::Image GetSwapChainImage(int index);
		vk::ImageView GetSwapChainImageView(int index);

		vk::ImageView GetDepthImageView();
		vk::Image GetDepthImage();

	private:

		Renderer* _renderer;

		vk::PresentModeKHR _present_mode;
		vk::Extent2D _extent;
		vk::raii::SwapchainKHR _swap_chain = nullptr;
		std::vector<vk::Image> _images;
		std::vector<vk::raii::ImageView> _image_views;
		vk::raii::Image _depth_image = nullptr;
		vk::raii::DeviceMemory _depth_image_memory = nullptr;
		vk::raii::ImageView _depth_image_view = nullptr;

		int _swap_chain_image_count;
	};
}