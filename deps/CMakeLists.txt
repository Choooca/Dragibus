cmake_minimum_required(VERSION 4.1.2)

include(FetchContent)

function(IncludeDependency lib_name git_url git_version sub_source_dir)
	message(STATUS "include ${lib_name}")

	FetchContent_Declare(
		${lib_name}
		GIT_REPOSITORY ${git_url}
		GIT_TAG ${git_version}
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		SOURCE_SUBDIR ${sub_source_dir}
	)

	FetchContent_MakeAvailable(${lib_name})
endfunction()

set(VULKAN_MAJOR_VERSION 1)
set(VULKAN_MINOR_VERSION 3)
set(VULKAN_PATCH_VERSION 278)
set(VULKAN_VERSION ${VULKAN_MAJOR_VERSION}.${VULKAN_MINOR_VERSION}.${VULKAN_PATCH_VERSION})

IncludeDependency(glfw https://github.com/glfw/glfw.git 3.4 "")
IncludeDependency(vulkanheaders https://github.com/KhronosGroup/Vulkan-Headers.git v${VULKAN_VERSION} "")
IncludeDependency(vulkanhpp https://github.com/KhronosGroup/Vulkan-Hpp.git v${VULKAN_VERSION} "")
IncludeDependency(glm https://github.com/g-truc/glm.git 1.0.3 "")
IncludeDependency(spdlog https://github.com/gabime/spdlog.git v1.17.0 "")

#I don't include stb with IncludeDependency because outside the function the first parameter doesn't seem to be a target.
message(STATUS "include stb_image")
FetchContent_Declare(
	stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG master
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

add_library(deps INTERFACE)
target_include_directories(deps INTERFACE ${vulkanhpp_SOURCE_DIR})
target_include_directories(deps INTERFACE ${stb_SOURCE_DIR})

find_package(Vulkan REQUIRED)
target_link_libraries(deps INTERFACE stb Vulkan::Headers Vulkan::Vulkan glfw glm::glm spdlog::spdlog)