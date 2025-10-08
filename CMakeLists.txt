cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(Piccolo VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disable compiler-specific extensions

include(CMakeDependentOption)

find_package(glfw3 REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(SOL2_INCLUDE_DIRS "sol/abort.hpp")
find_package(Stb REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)

find_package(Taskflow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# target_link_libraries(main PRIVATE nlohmann_json::nlohmann_json)
find_package(EnTT CONFIG REQUIRED)
# target_link_libraries(main PRIVATE EnTT::EnTT)
find_path(REFL_CPP_INCLUDE_DIRS "refl.hpp")
# target_include_directories(main PRIVATE ${REFL_CPP_INCLUDE_DIRS})
find_package(reflectcpp CONFIG REQUIRED)
# target_link_libraries(main PRIVATE reflectcpp::reflectcpp)
find_package(glm CONFIG REQUIRED)
# target_link_libraries(main PRIVATE glm::glm-header-only)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

set(PICCOLO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_INSTALL_PREFIX "${PICCOLO_ROOT_DIR}/bin")
set(BINARY_ROOT_DIR "${CMAKE_INSTALL_PREFIX}")

add_subdirectory(engine)
