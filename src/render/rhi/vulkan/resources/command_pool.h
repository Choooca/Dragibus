#pragma once;

#include <vector>
#include <map>
#include <string>

#include <vulkan/vulkan.hpp>


namespace Vulkan {

	class CommandPool {

	public:

		CommandPool(VkDevice *device, uint32_t queue_family_index, VkCommandPoolCreateFlagBits flags);
		~CommandPool();

		std::vector<VkCommandBuffer> CreateCommandBufferGroup(uint32_t count, const std::string &key);
		void DestroyCommandBufferGroup(const std::string &key);

		VkCommandPool Get();

		VkCommandBuffer GetCommandBuffer(const std::string& key, uint32_t index);

	private:

		VkCommandPool m_command_pool;
		VkDevice* m_device;

		std::map<std::string ,std::vector<VkCommandBuffer>> m_command_buffers_groups;

	};

}