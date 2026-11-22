#include "swap_chain_resources.h"

#include <algorithm>
#include <ranges>

#include <render/rhi/vulkan/renderer.h>
#include <render/rhi/vulkan/resource_utils.h>
#include "descriptor_layouts.h"

Vulkan::SwapChainResources::SwapChainResources(const Vulkan::Context& context, vk::Sampler post_process_sampler, vk::DescriptorSetLayout tonemap_descriptor_set_layout)
	: _context(context)
{

	// ----------------- Present Mode -----------------

	_present_mode = std::ranges::any_of(_context.GetAvailablePresentModes(),
		[](auto const& present_mode) { return present_mode == vk::PresentModeKHR::eMailbox; }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;

	// ----------------- Swap Chain Extent -----------------

	vk::SurfaceCapabilitiesKHR surface_capabilities = _context.GetSurfaceCapabilities();
	if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		_extent = surface_capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(_context.GetWindow(), &width, &height);

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

	vk::SwapchainCreateInfoKHR swap_chain_create_info{ .surface = _context.GetSurface(),
											   .minImageCount = min_image_count,
											   .imageFormat = _context.GetSurfaceFormat().format,
											   .imageColorSpace = _context.GetSurfaceFormat().colorSpace,
											   .imageExtent = _extent,
											   .imageArrayLayers = 1,
											   .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
											   .imageSharingMode = vk::SharingMode::eExclusive,
											   .preTransform = surface_capabilities.currentTransform,
											   .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
											   .presentMode = _present_mode,
											   .clipped = true };

	_swap_chain = vk::raii::SwapchainKHR(_context.GetDevice(), swap_chain_create_info);

	// ----------------- Images & Image Views -----------------

	_images = _swap_chain.getImages();
	_swap_chain_image_count = _images.size();

	_image_views.reserve(_images.size());
	for (auto& image : _images) {
		_image_views.emplace_back(CreateImageView(context, image, _context.GetSurfaceFormat().format, vk::ImageAspectFlagBits::eColor));
	}

	std::tie(_offscreen_image, _offscreen_memory) = CreateImage(
		_context, _extent.width, _extent.height,
		Renderer::HDR_FORMAT, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

	_offscreen_view = CreateImageView(_context, _offscreen_image, Renderer::HDR_FORMAT, vk::ImageAspectFlagBits::eColor);

	std::tie(_depth_image, _depth_image_memory) = CreateImage(context, _extent.width, _extent.height,
		context.GetDepthFormat(), vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);

	_depth_image_view = CreateImageView(context, _depth_image, context.GetDepthFormat(), vk::ImageAspectFlagBits::eDepth);

	vk::DescriptorImageInfo offscreen_image_info = {
		.sampler = post_process_sampler,
		.imageView = _offscreen_view,
		.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	};

	DescriptorSetGroupDesc tonemap_descriptor_desc = {
		.descriptor_set_layout = tonemap_descriptor_set_layout,
		.bindings = TONEMAPPING_BINDINGS,
		.resources = { { { std::vector{ offscreen_image_info } } } }
	};
	_tonemap_descriptor_sets = CreateDescriptorSetGroup(_context, tonemap_descriptor_desc);
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

vk::Image Vulkan::SwapChainResources::GetOffScreenImage() { return _offscreen_image; }

vk::ImageView Vulkan::SwapChainResources::GetOffScreenImageView() { return _offscreen_view; }

vk::Image Vulkan::SwapChainResources::GetDepthImage() { return _depth_image; }

vk::ImageView Vulkan::SwapChainResources::GetDepthImageView() { return _depth_image_view; }

vk::DescriptorSet Vulkan::SwapChainResources::GetTonemapDescriptorSet() const { return _tonemap_descriptor_sets.sets[0]; }
