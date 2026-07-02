#include "texture.h"

#include <stb_image.h>

#include <utils/build_macro.h>
#include <render/rhi/vulkan/resources/buffer.h>
#include <render/rhi/vulkan/resources/device_memory.h>
#include <render/rhi/vulkan/resources/image.h>
#include <render/rhi/vulkan/renderer.h>
#include <utils/custom_type.h>

Vulkan::Texture::Texture(Renderer* renderer, const std::string& texture_name)
	: m_renderer(renderer)
{
	int tex_width, tex_height, tex_channels;

	std::string complete_path = std::string(TEXTURES_DIR) + texture_name;
	stbi_uc* pixels = stbi_load(complete_path.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	VkDeviceSize size = tex_height * tex_width * 4;

	VkMemoryPropertyFlags staging_buffer_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	Buffer staging_buffer = Buffer(m_renderer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_buffer_properties);
	VkMemoryRequirements staging_mem_requirements{};
	vkGetBufferMemoryRequirements(m_renderer->GetDevice(), staging_buffer.Get(), &staging_mem_requirements);
	DeviceMemory staging_buffer_memory = DeviceMemory(m_renderer, staging_mem_requirements, staging_buffer_properties);
	vkBindBufferMemory(m_renderer->GetDevice(), staging_buffer.Get(), staging_buffer_memory.Get(), 0);

	void* data;
	vkMapMemory(m_renderer->GetDevice(), staging_buffer_memory.Get(), 0, size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(size));
	vkUnmapMemory(m_renderer->GetDevice(), staging_buffer_memory.Get());

	stbi_image_free(pixels);
	
	VkMemoryPropertyFlags image_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	m_image = std::make_unique<Image>(m_renderer, tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	VkMemoryRequirements image_mem_requirements{};
	vkGetImageMemoryRequirements(m_renderer->GetDevice(), m_image->Get(), &image_mem_requirements);
	m_device_memory = std::make_unique<DeviceMemory>(m_renderer, image_mem_requirements, image_memory_properties);
	vkBindImageMemory(m_renderer->GetDevice(), m_image->Get(), m_device_memory->Get(), 0);

	m_image->TransitionImageLayout(m_renderer->GetTransferCommandPool(), m_renderer->GetTransferQueue(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_image->CopyBufferToImage(staging_buffer.Get(), tex_width, tex_height);
	m_image->TransitionImageLayout(m_renderer->GetGraphicsCommandPool(), m_renderer->GetGraphicsQueue(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

Vulkan::Texture::~Texture()
{
}

VkImage Vulkan::Texture::GetImage()
{
	return m_image->Get();
}
