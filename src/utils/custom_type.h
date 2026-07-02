#pragma once;
#include <optional>
#include <glm/glm.hpp>

struct TransformationMatrix {
	glm::vec3 model;
	glm::vec3 view;
	glm::vec3 proj;
};

namespace Vulkan {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;
		std::optional<uint32_t> transfer_family;

		bool IsComplete() {
			return graphics_family.has_value() && present_family.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};
}