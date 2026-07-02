cmake_minimum_required(VERSION 4.1.2)

target_sources(Dragibus
	PRIVATE 
		debug_messenger.cpp
		device.cpp
		instance.cpp
		surface.cpp
		swap_chain.cpp
		render_pass.cpp
		descriptor_set_layout.cpp
		pipeline_layout.cpp
		graphics_pipeline.cpp
		framebuffer.cpp
)