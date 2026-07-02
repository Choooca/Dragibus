#pragma once

#include <vulkan/vulkan.hpp>

struct RenderContext;

namespace Vulkan {

	struct SwapChainSupportDetails;
	struct QueueFamilyIndices;
	class Renderer;

	class SwapChain {

	public:

		SwapChain(
			Renderer *renderer,
			const VkExtent2D& swap_chain_extents,
			const VkPresentModeKHR& swap_chain_present_mode,
			const SwapChainSupportDetails& swap_chain_support_details);

		~SwapChain();

		VkSwapchainKHR Get();

	private:

		VkSwapchainKHR m_swap_chain = VK_NULL_HANDLE;
		Renderer* m_renderer;

	};
}