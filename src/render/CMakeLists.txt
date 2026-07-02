cmake_minimum_required(VERSION 4.1.2)

add_subdirectory(rhi)

target_sources(Dragibus
	PRIVATE 
		window.cpp
)