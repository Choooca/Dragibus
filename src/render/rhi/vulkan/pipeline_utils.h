#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
	class Context;

	struct GraphicsPipelineDesc {
		vk::PipelineLayout pipeline_layout;
		vk::Format attachment_format;
		vk::Format depth_format;
		std::vector<vk::VertexInputAttributeDescription> vertex_input_attributes;
		std::vector<vk::VertexInputBindingDescription> vertex_input_bindings;
		std::string file_name;
	};

	vk::raii::Pipeline CreateGraphicsPipeline(const Vulkan::Context& context, const GraphicsPipelineDesc& desc);
}