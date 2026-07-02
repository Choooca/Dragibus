#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Vulkan {

	class Renderer;

	class Framebuffer {

	public:

		Framebuffer(Renderer *renderer, const VkImageView& color_image_view, const VkImageView& depth_image_view, const VkExtent2D extent);
		~Framebuffer();

		VkFramebuffer Get();

	private:

		VkFramebuffer m_framebuffer;
		Renderer *m_renderer;
	};

}