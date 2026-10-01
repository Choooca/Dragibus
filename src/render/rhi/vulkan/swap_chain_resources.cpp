#include "swap_chain_resources.h"

#include <algorithm>
#include <array>

#include <utils/debug_macro.h>
#include <utils/files.h>
#include <utils/build_macro.h>
#include <render/primitives.h>
#include <render/rhi/vulkan/renderer.h>
#include <render/rhi/vulkan/resources/image_view.h>
#include <render/rhi/vulkan/resources/image.h>
#include <render/rhi/vulkan/resources/device_memory.h>
#include <render/rhi/vulkan/handles/swap_chain.h>
#include <render/rhi/vulkan/handles/framebuffer.h>
#include <render/rhi/vulkan/handles/render_pass.h>

Vulkan::SwapChainResources::SwapChainResources(Renderer* renderer)
	: _renderer(renderer)
{
	SwapChainSupportDetails swap_chain_support_details = renderer->GetSwapChainSupportDetails(renderer->GetPhysicalDevice(), renderer->GetSurface());
	_present_mode = ChooseSwapChainPresentMode(swap_chain_support_details);
	_extent = GetSwapChainExtent(swap_chain_support_details);
	_swap_chain = std::make_unique<SwapChain>(renderer, _extent, _present_mode, swap_chain_support_details);
	_images = RetrieveSwapChainImage(_swap_chain->Get());
	_swap_chain_image_count = _images.size();
	_image_views = CreateSwapChainImageViews(_images, VK_IMAGE_ASPECT_COLOR_BIT);
	_depth_image = std::make_unique<Image>(renderer, _extent.width, _extent.height, renderer->GetDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	VkMemoryRequirements depth_mem_requirements{};
	vkGetImageMemoryRequirements(renderer->GetDevice(), _depth_image->Get(), &depth_mem_requirements);
	_depth_image_memory = std::make_unique<DeviceMemory>(_renderer, depth_mem_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkBindImageMemory(_renderer->GetDevice(), _depth_image->Get(), _depth_image_memory->Get(), 0);
	_depth_image_view = std::make_unique<ImageView>(_renderer, _depth_image->Get(), _renderer->GetDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);
	_depth_image->TransitionImageLayout(_renderer->GetGraphicsCommandPool(), _renderer->GetGraphicsQueue(), _renderer->GetDepthFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	_framebuffers.reserve(_swap_chain_image_count);
	for (int i = 0; i < _swap_chain_image_count; ++i) {
		_framebuffers.emplace_back(renderer, _image_views[i].Get(), _depth_image_view->Get(), _extent);
	}
}

Vulkan::SwapChainResources::~SwapChainResources(){}

int Vulkan::SwapChainResources::GetSwapChainImageCount() { return _swap_chain_image_count; }

VkExtent2D Vulkan::SwapChainResources::GetSwapchainExtent() { return _extent; }

VkSwapchainKHR Vulkan::SwapChainResources::GetSwapchain() { return _swap_chain->Get(); }

VkFramebuffer Vulkan::SwapChainResources::GetFramebuffer(int swap_chain_index) { return _framebuffers[swap_chain_index].Get(); }

VkPresentModeKHR Vulkan::SwapChainResources::ChooseSwapChainPresentMode(const SwapChainSupportDetails& swap_chain_support_details)
{
	const std::vector<VkPresentModeKHR>& available_present_modes = swap_chain_support_details.present_modes;

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (const VkPresentModeKHR& available_present_mode : available_present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = available_present_mode;
		}
	}

	return present_mode;
}

VkExtent2D Vulkan::SwapChainResources::GetSwapChainExtent(const SwapChainSupportDetails& swap_chain_support_details)
{
	VkExtent2D image_extent;
	const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		image_extent = capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(_renderer->GetWindow(), &width, &height);

		image_extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		image_extent.width = std::clamp(image_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		image_extent.height = std::clamp(image_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}

	return image_extent;
}

std::vector<VkImage> Vulkan::SwapChainResources::RetrieveSwapChainImage(const VkSwapchainKHR swap_chain)
{
	uint32_t swap_chain_image_count;
	vkGetSwapchainImagesKHR(_renderer->GetDevice(), swap_chain, &swap_chain_image_count, nullptr);

	std::vector<VkImage> swap_chain_images(swap_chain_image_count);
	vkGetSwapchainImagesKHR(_renderer->GetDevice(), swap_chain, &swap_chain_image_count, swap_chain_images.data());

	return swap_chain_images;
}

std::vector<Vulkan::ImageView> Vulkan::SwapChainResources::CreateSwapChainImageViews(const std::vector<VkImage> swap_chain_images, const VkImageAspectFlags aspect_flags)
{
	size_t swap_chain_image_count = swap_chain_images.size();
	std::vector<ImageView> swap_chain_image_views;
	swap_chain_image_views.reserve(swap_chain_image_count);

	for (size_t i = 0; i < swap_chain_image_count; ++i) {
		swap_chain_image_views.emplace_back(_renderer, swap_chain_images[i], _renderer->GetSurfaceFormat().format, aspect_flags);
	}

	return swap_chain_image_views;
}
