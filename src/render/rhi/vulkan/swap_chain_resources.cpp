#include "swap_chain_resources.h"

#include <algorithm>
#include <ranges>

#include <render/rhi/vulkan/renderer.h>
#include <render/rhi/vulkan/resource_utils.h>

Vulkan::SwapChainResources::SwapChainResources(Renderer* renderer)
	: _renderer(renderer)
{

	// ----------------- Present Mode -----------------

	_present_mode = std::ranges::any_of(_renderer->GetAvailablePresentModes(),
		[](auto const& present_mode) { return present_mode == vk::PresentModeKHR::eMailbox; }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;

	// ----------------- Swap Chain Extent -----------------

	vk::SurfaceCapabilitiesKHR surface_capabilities = _renderer->GetSurfaceCapabilities();
	if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		_extent = surface_capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(_renderer->GetWindow(), &width, &height);

		_extent = {
			std::clamp<uint32_t>(width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height)
		};
	}

	// ----------------- Swap Chain Image Count -----------------

	uint32_t min_image_count = std::max(3u, surface_capabilities.minImageCount);
	if (0 < surface_capabilities.maxImageCount && surface_capabilities.maxImageCount < min_image_count) {
		min_image_count = surface_capabilities.maxImageCount;
	}

	// ----------------- Swap Chain -----------------

	vk::SwapchainCreateInfoKHR swap_chain_create_info{ .surface = renderer->GetSurface(),
											   .minImageCount = min_image_count,
											   .imageFormat = renderer->GetSurfaceFormat().format,
											   .imageColorSpace = renderer->GetSurfaceFormat().colorSpace,
											   .imageExtent = _extent,
											   .imageArrayLayers = 1,
											   .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
											   .imageSharingMode = vk::SharingMode::eExclusive,
											   .preTransform = surface_capabilities.currentTransform,
											   .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
											   .presentMode = _present_mode,
											   .clipped = true };

	_swap_chain = vk::raii::SwapchainKHR(_renderer->GetDevice(), swap_chain_create_info);

	// ----------------- Images & Image Views -----------------

	_images = _swap_chain.getImages();
	_swap_chain_image_count = _images.size();

	_image_views.reserve(_images.size());
	for (auto& image : _images) {
		_image_views.emplace_back(CreateImageView(renderer, image, _renderer->GetSurfaceFormat().format, vk::ImageAspectFlagBits::eColor));
	}

	std::tie(_depth_image, _depth_image_memory) = CreateImage(renderer, _extent.width, _extent.height, renderer->GetDepthFormat(), vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
	_depth_image_view = CreateImageView(renderer, _depth_image, renderer->GetDepthFormat(), vk::ImageAspectFlagBits::eDepth);
}

Vulkan::SwapChainResources::~SwapChainResources(){}

int Vulkan::SwapChainResources::GetSwapChainImageCount() { return _swap_chain_image_count; }

vk::Extent2D &Vulkan::SwapChainResources::GetSwapChainExtent()
{
	return _extent;
}

vk::raii::SwapchainKHR &Vulkan::SwapChainResources::GetSwapchain() { return _swap_chain; }


vk::Image Vulkan::SwapChainResources::GetSwapChainImage(int index)
{
	return _images[index];
}

vk::ImageView Vulkan::SwapChainResources::GetSwapChainImageView(int index)
{
	return _image_views[index];
}

vk::ImageView Vulkan::SwapChainResources::GetDepthImageView()
{
	return _depth_image_view;
}

vk::Image Vulkan::SwapChainResources::GetDepthImage()
{
	return _depth_image;
}
