#pragma once

struct VkContext;
struct SwapChain;
struct Renderer;

void DrawFrame(const VkContext& vk_context, SwapChain& swap_chain, Renderer& renderer);