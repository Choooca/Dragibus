cmake_minimum_required(VERSION 4.1.2)

target_sources(Dragibus
	PRIVATE 
		window.cpp
		vk_context.cpp
		vk_render_context.cpp
		vk_render.cpp
		vk_utils.cpp
		primitives.h
)