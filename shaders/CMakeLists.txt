cmake_minimum_required(VERSION 4.1.2)

find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/Bin" REQUIRED)

set(SHADER_SLANG_SOURCES
	simple_shader.slang
)

set(SHADER_SPIRV_OUTPUTS)
foreach(shader_source ${SHADER_SLANG_SOURCES})
	get_filename_component(shader_name ${shader_source} NAME_WE)
	set(shader_input  ${CMAKE_CURRENT_SOURCE_DIR}/${shader_source})
	set(shader_output ${CMAKE_CURRENT_SOURCE_DIR}/${shader_name}.spv)

	add_custom_command(
		OUTPUT ${shader_output}
		COMMAND ${SLANGC_EXECUTABLE} ${shader_input}
			-target spirv -profile spirv_1_4 -emit-spirv-directly
			-fvk-use-entrypoint-name -entry VertMain -entry FragMain
			-o ${shader_output}
		DEPENDS ${shader_input}
		COMMENT "Compiling Slang shader ${shader_source}"
		VERBATIM
	)

	list(APPEND SHADER_SPIRV_OUTPUTS ${shader_output})
endforeach()

add_custom_target(DragibusShaders DEPENDS ${SHADER_SPIRV_OUTPUTS})
add_dependencies(Dragibus DragibusShaders)
