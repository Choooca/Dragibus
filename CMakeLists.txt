cmake_minimum_required(VERSION 4.1.2)

project(
	Dragibus 
	VERSION 1.0.0	
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Dragibus)

add_subdirectory(deps)
add_subdirectory(shaders)
add_subdirectory(src)

if(MSVC)
    target_compile_options(Dragibus PRIVATE /Zc:preprocessor)
endif()

target_link_libraries(Dragibus 
	PRIVATE
		deps
)

target_compile_definitions(Dragibus PRIVATE GLFW_INCLUDE_VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(
	Dragibus
	PRIVATE
		${CMAKE_SOURCE_DIR}/src
		${CMAKE_CURRENT_BINARY_DIR}/src
)
		
		