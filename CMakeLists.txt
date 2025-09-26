cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(Piccolo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

include(CMakeDependentOption)

find_package(glfw3 REQUIRED)
find_package(spdlog CONFIG REQUIRED)
# target_link_libraries(main PRIVATE spdlog::spdlog)
# Or use the header-only version
# target_link_libraries(main PRIVATE spdlog::spdlog_header_only)

# find_package(sol2 REQUIRED)
find_path(SOL2_INCLUDE_DIRS "sol/abort.hpp")
# target_include_directories(main PRIVATE ${SOL2_INCLUDE_DIRS})
find_package(Stb REQUIRED)
# target_include_directories(main PRIVATE ${Stb_INCLUDE_DIR})
find_package(Jolt CONFIG REQUIRED)
# target_link_libraries(main PRIVATE Jolt::Jolt)
find_package(Lua REQUIRED)
# target_include_directories(main PRIVATE ${LUA_INCLUDE_DIR})
# target_link_libraries(main PRIVATE ${LUA_LIBRARIES})
find_package(glslang CONFIG REQUIRED)
# target_link_libraries(main PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)
find_package(volk CONFIG REQUIRED)
# target_link_libraries(main PRIVATE volk::volk volk::volk_headers)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
# target_include_directories(main PRIVATE ${TINYGLTF_INCLUDE_DIRS})
find_package(tinyobjloader CONFIG REQUIRED)
# target_link_libraries(main PRIVATE tinyobjloader::tinyobjloader)

# find_package(VulkanHeaders CONFIG)
# target_link_libraries(main PRIVATE Vulkan::Headers)
# this is heuristically generated, and may not be correct
# find_package(VulkanUtilityLibraries CONFIG REQUIRED)
# target_link_libraries(main PRIVATE Vulkan::SafeStruct Vulkan::LayerSettings Vulkan::UtilityHeaders Vulkan::CompilerConfiguration)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# find_package(Vulkan) # https://cmake.org/cmake/help/latest/module/FindVulkan.html, CMake 3.21+
# target_link_libraries(main PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)

# find_package(Vulkan) # CMake 3.21+
# find_package(VulkanMemoryAllocator CONFIG REQUIRED)
# target_link_libraries(main PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator)

# find_package(VulkanHeaders CONFIG) # From the vulkan-headers port
# find_package(VulkanMemoryAllocator CONFIG REQUIRED)
# target_link_libraries(main PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

set(PICCOLO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_INSTALL_PREFIX "${PICCOLO_ROOT_DIR}/bin")
set(BINARY_ROOT_DIR "${CMAKE_INSTALL_PREFIX}/")

add_subdirectory(engine)
