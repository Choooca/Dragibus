#include "vk_utils.h"

#include <render/vk_context.h>
#include <utils/debug_macro.h>

SwapChainSupportDetails GetSwapChainSupportDetails(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface)
{
	SwapChainSupportDetails swap_chain_support_details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &swap_chain_support_details.capabilities);

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
	swap_chain_support_details.formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, swap_chain_support_details.formats.data());

	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
	swap_chain_support_details.present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, swap_chain_support_details.present_modes.data());

	return swap_chain_support_details;
}