cmake_minimum_required(VERSION 4.1.2)

add_subdirectory(vulkan)

target_sources(Dragibus
	PRIVATE
		gltf_model.cpp
		tiny_gltf_v3_impl.cpp
)