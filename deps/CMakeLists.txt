cmake_minimum_required(VERSION 4.1.2)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(mikktspace CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_path(TINYGLTF_INCLUDE_DIR tiny_gltf_v3.h REQUIRED)

add_library(deps INTERFACE)
target_include_directories(deps INTERFACE ${Stb_INCLUDE_DIR})
target_include_directories(deps INTERFACE ${TINYGLTF_INCLUDE_DIR})

target_compile_definitions(deps INTERFACE VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
target_compile_definitions(deps INTERFACE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS)

target_link_libraries(deps INTERFACE Vulkan::Vulkan glfw glm::glm spdlog::spdlog mikktspace::mikktspace)