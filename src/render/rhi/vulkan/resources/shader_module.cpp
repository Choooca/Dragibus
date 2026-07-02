#include "shader_module.h"

#include <utils/files.h>
#include <utils/build_macro.h>
#include <utils/debug_macro.h>
#include <render/rhi/vulkan/renderer.h>

Vulkan::ShaderModule::ShaderModule(Renderer* renderer, const std::string& shader_file_name)
	: m_renderer(renderer)
{
	std::vector<char> shader_code = ReadFile(std::string(SHADERS_DIR) + shader_file_name);

	VkShaderModuleCreateInfo module_info{};
	module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_info.codeSize = shader_code.size();
	module_info.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

	if (vkCreateShaderModule(m_renderer->GetDevice(), &module_info, nullptr, &m_shader_module) != VK_SUCCESS) {
		THROW_RUNTIME_ERROR("Failed to create shader module " + shader_file_name);
	}
}

Vulkan::ShaderModule::~ShaderModule()
{
	vkDestroyShaderModule(m_renderer->GetDevice(), m_shader_module, nullptr);
}

VkShaderModule Vulkan::ShaderModule::Get()
{
	return m_shader_module;
}
