#pragma once

#include <memory>
#include <string>

#include <vulkan/vulkan.hpp>


namespace Vulkan {

	class PipelineLayout;
	class Renderer;

	class GraphicsPipeline {

	public:

		GraphicsPipeline(Renderer* renderer, const std::string& vert_shader_name, const std::string& frag_shader_name);
		~GraphicsPipeline();

		VkPipeline Get();

		VkPipelineLayout GetPipelineLayout();

	private:

		VkPipeline m_pipeline;
		Renderer *m_renderer;

		std::unique_ptr<PipelineLayout> m_pipeline_layout;
	};

}