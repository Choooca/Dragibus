cmake_minimum_required(VERSION 4.1.2)

target_sources(Dragibus
	PRIVATE
		main.cpp
)

add_subdirectory(core)
add_subdirectory(render)
add_subdirectory(utils)