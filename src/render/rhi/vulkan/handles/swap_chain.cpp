#include "swap_chain.h"
#include <utils/debug_macro.h>
#include <utils/custom_type.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::SwapChain::SwapChain(
	Renderer* renderer,
	const VkExtent2D& swap_chain_extents,
	const VkPresentModeKHR& swap_chain_present_mode,
	const SwapChainSupportDetails& swap_chain_support_details
)
	: _renderer(renderer)
{
	const VkSurfaceCapabilitiesKHR& capabilities = swap_chain_support_details.capabilities;

	uint32_t image_count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < image_count) {
		image_count = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swap_chain_info{};
	swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_chain_info.surface = _renderer->GetSurface();
	swap_chain_info.minImageCount = image_count;
	swap_chain_info.imageFormat = _renderer->GetSurfaceFormat().format;
	swap_chain_info.imageColorSpace = _renderer->GetSurfaceFormat().colorSpace;
	swap_chain_info.imageExtent = swap_chain_extents;
	swap_chain_info.imageArrayLayers = 1;
	swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swap_chain_info.presentMode = swap_chain_present_mode;

	uint32_t image_family_indices[] = { 
		_renderer->GetQueueFamilyIndices().graphics_family.value(),
		_renderer->GetQueueFamilyIndices().present_family.value()
	};

	if (
		_renderer->GetQueueFamilyIndices().graphics_family.value() !=
		_renderer->GetQueueFamilyIndices().present_family.value()
		) {
		swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swap_chain_info.queueFamilyIndexCount = 2;
		swap_chain_info.pQueueFamilyIndices = image_family_indices;
	}
	else {
		swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swap_chain_info.preTransform = capabilities.currentTransform;
	swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_chain_info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(_renderer->GetDevice(), &swap_chain_info, nullptr, &_swap_chain) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create swap chain");
	}
}

Vulkan::SwapChain::~SwapChain()
{
	vkDestroySwapchainKHR(_renderer->GetDevice(), _swap_chain, nullptr);
}

VkSwapchainKHR Vulkan::SwapChain::Get()
{
	return _swap_chain;
}
