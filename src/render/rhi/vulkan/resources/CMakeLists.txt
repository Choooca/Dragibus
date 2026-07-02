cmake_minimum_required(VERSION 4.1.2)

target_sources(
	Dragibus
	PRIVATE
		image_view.cpp
		device_memory.cpp
		buffer.cpp
		uniform_buffer.cpp
		descriptor_pool.cpp
		shader_module.cpp
		command_pool.cpp
		primitive_buffer.cpp
		image.cpp
		texture.cpp
		sampler.cpp
		semaphore.cpp
		fence.cpp
)