cmake_minimum_required(VERSION 4.1.2)

target_sources(Dragibus
	PRIVATE
		renderer.cpp
		swap_chain_resources.cpp
		frame_resources.cpp
		command_utils.cpp
		resource_utils.cpp
		model.cpp
		context.cpp
)

