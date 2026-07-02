#pragma once

#include <string>

#include <vulkan/vulkan.hpp>

namespace Vulkan {

	class Renderer;

	class ShaderModule {

	public:

		ShaderModule(Renderer *renderer, const std::string& shader_file_name);
		~ShaderModule();

		VkShaderModule Get();

	private:

		VkShaderModule m_shader_module;
		Renderer *m_renderer;

	};

}