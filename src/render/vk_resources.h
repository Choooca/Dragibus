#pragma once
#include <vector>

#include <glfw/glfw3.h>

#include <render/vk_context.h>

//Synchrinisation
std::vector<VkSemaphore> CreateSemaphores(const VkContext& context, uint32_t count, VkSemaphoreCreateFlags flags = 0);
void DestroySemaphores(const VkContext& context, std::vector<VkSemaphore>& semaphore);

std::vector<VkFence> CreateFences(const VkContext& context, uint32_t count, VkFenceCreateFlags flags = 0);
void DestroyFences(const VkContext& context, std::vector<VkFence>& fence);

//Buffers
uint32_t FindMemoryType(const VkContext& context, uint32_t type_filter, VkMemoryPropertyFlags properties);
void CreateBuffer(const VkContext& vk_context, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
void CopyBuffer(const VkContext& vk_context, const VkCommandPool& command_pool, const VkBuffer& src_buffer, const VkBuffer& dst_buffer, VkDeviceSize size);

//Command Buffer

VkCommandPool CreateCommandPool(const VkContext& vk_context, uint32_t queue_family_index, VkCommandPoolCreateFlagBits flags);
std::vector<VkCommandBuffer> CreateCommandBuffer(const VkContext& vk_context, const VkCommandPool& command_pool, uint32_t count);
VkCommandBuffer BeginSingleCommandBuffer(const VkContext& vk_context, const VkCommandPool& command_pool);
void EndSingleCommandBuffer(const VkContext& vk_context, const VkCommandBuffer& command_buffer, const VkCommandPool& command_pool, const VkQueue& queue);
