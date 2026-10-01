#include "primitive_buffer.h"

VkBuffer Vulkan::PrimitiveBuffer::GetBuffer() { return _buffer->Get(); }
