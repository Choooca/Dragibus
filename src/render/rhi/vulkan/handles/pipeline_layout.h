#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Renderer;

	class PipelineLayout {

	public:

		PipelineLayout(Renderer* renderer);
		~PipelineLayout();

		VkPipelineLayout Get();

	private:

		VkPipelineLayout m_pipeline_layout;
		Renderer *m_renderer;

	};

}