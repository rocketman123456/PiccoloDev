cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(Piccolo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

include(CMakeDependentOption)

find_package(glfw3 REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(SOL2_INCLUDE_DIRS "sol/abort.hpp")
find_package(Stb REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

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
