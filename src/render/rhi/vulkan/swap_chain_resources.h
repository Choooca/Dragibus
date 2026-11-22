#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <render/rhi/vulkan/context.h>
#include <render/rhi/vulkan/descriptor_utils.h>

namespace Vulkan {

	class SwapChainResources {

	public:
		SwapChainResources(const Vulkan::Context &context, vk::Sampler post_process_sampler, vk::DescriptorSetLayout tonemap_descriptor_set_layout);
		~SwapChainResources();

		int GetSwapChainImageCount();
		vk::Extent2D &GetSwapChainExtent();

		bool _frame_buffer_resized = false;

		vk::raii::SwapchainKHR &GetSwapchain();

		vk::Image GetSwapChainImage(int index);
		vk::ImageView GetSwapChainImageView(int index);

		vk::Image GetOffScreenImage();
		vk::ImageView GetOffScreenImageView();

		vk::Image GetDepthImage();
		vk::ImageView GetDepthImageView();

		vk::DescriptorSet GetTonemapDescriptorSet() const;

	private:

		const Vulkan::Context &_context;

		vk::PresentModeKHR _present_mode;
		vk::Extent2D _extent;
		vk::raii::SwapchainKHR _swap_chain = nullptr;
		std::vector<vk::Image> _images;
		std::vector<vk::raii::ImageView> _image_views;

		vk::raii::Image _depth_image = nullptr;
		vk::raii::DeviceMemory _depth_image_memory = nullptr;
		vk::raii::ImageView _depth_image_view = nullptr;

		vk::raii::Image _offscreen_image = nullptr;
		vk::raii::DeviceMemory _offscreen_memory = nullptr;
		vk::raii::ImageView _offscreen_view = nullptr;

		DescriptorSetGroup _tonemap_descriptor_sets;

		int _swap_chain_image_count;
	};
}