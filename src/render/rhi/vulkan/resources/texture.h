#pragma once

#include <memory>
#include <string>

#include <vulkan/vulkan.h>

namespace Vulkan {

	class Image;
	class DeviceMemory;
	class CommandPool;
	class Renderer;
	struct QueueFamilyIndices;

	class Texture {
		
	public:

		Texture(Renderer *renderer, const std::string& texture_name);
		~Texture();

		VkImage GetImage();

	private:

		Renderer* m_renderer;

		std::unique_ptr<Image> m_image;
		std::unique_ptr<DeviceMemory> m_device_memory;
	};

}