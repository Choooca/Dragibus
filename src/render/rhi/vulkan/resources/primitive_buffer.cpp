#include "primitive_buffer.h"

VkBuffer Vulkan::PrimitiveBuffer::GetBuffer() { return m_buffer->Get(); }
