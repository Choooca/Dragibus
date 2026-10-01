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

		Renderer* _renderer;

		std::unique_ptr<Image> _image;
		std::unique_ptr<DeviceMemory> _device_memory;
	};

}