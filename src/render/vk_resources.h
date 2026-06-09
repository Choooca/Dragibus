#pragma once
#include <vector>

#include <glfw/glfw3.h>

struct Renderer;
struct VkContext;

//Synchronisation
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

//Images
void CreateImage(const VkContext& vk_context, uint32_t width, uint32_t height, const VkFormat& format, const VkImageTiling& tilling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);
void TransitionImageLayout(const VkContext vk_context, const Renderer& renderer, VkImage& image, const VkFormat& format, const VkImageLayout& old_layout, const VkImageLayout& new_layout);

//Textures
void CreateTextureImage();