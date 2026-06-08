cmake_minimum_required(VERSION 4.1.2)

target_sources(Dragibus
	PRIVATE 
		window.cpp
		vk_context.cpp
		swap_chain.cpp
		vk_resources.cpp
		renderer.cpp
		vk_utils.cpp
		primitives.h
)