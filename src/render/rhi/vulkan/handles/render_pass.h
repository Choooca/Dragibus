#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;

	class RenderPass {

	public:

		RenderPass(Renderer *renderer);
		~RenderPass();

		VkRenderPass Get();

	private:

		VkRenderPass _render_pass;
		Renderer *_renderer;
	};
}