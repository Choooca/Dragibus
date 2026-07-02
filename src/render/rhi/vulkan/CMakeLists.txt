cmake_minimum_required(VERSION 4.1.2)

add_subdirectory(handles)
add_subdirectory(resources)

target_sources(Dragibus
	PRIVATE
		renderer.cpp
		render_loop.cpp
		swap_chain_resources.cpp
		frame_resources.cpp
)

