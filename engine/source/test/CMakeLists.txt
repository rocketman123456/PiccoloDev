include(${PICCOLO_ROOT_DIR}/cmake/install.cmake)

add_executable(taskflow_test lib_test/taskflow_test.cpp)
target_link_libraries(taskflow_test PRIVATE Taskflow::Taskflow)
auto_install_target(taskflow_test ${BINARY_ROOT_DIR})

# add_executable(shader_compile_test lib_test/shader_compile_test.cpp)
# target_link_libraries(shader_compile_test PRIVATE PiccoloRuntime)
# auto_install_target(shader_compile_test ${BINARY_ROOT_DIR})

add_executable(reflection_test lib_test/reflection_test.cpp)
target_link_libraries(reflection_test PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(reflection_test PRIVATE EnTT::EnTT)
target_include_directories(reflection_test PRIVATE ${REFL_CPP_INCLUDE_DIRS})
auto_install_target(reflection_test ${BINARY_ROOT_DIR})

add_executable(reflection_test_2 lib_test/reflection_test_2.cpp)
target_link_libraries(reflection_test_2 PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(reflection_test_2 PRIVATE EnTT::EnTT)
target_include_directories(reflection_test_2 PRIVATE ${REFL_CPP_INCLUDE_DIRS})
target_link_libraries(reflection_test_2 PRIVATE reflectcpp::reflectcpp)
auto_install_target(reflection_test_2 ${BINARY_ROOT_DIR})

# 原始版本（保留作为备份）
add_executable(jolt_character_test lib_test/jolt_character_test.cpp)
target_link_libraries(jolt_character_test PRIVATE glfw)
target_link_libraries(jolt_character_test PRIVATE Jolt::Jolt)
target_link_libraries(jolt_character_test PRIVATE glm::glm)
target_link_libraries(jolt_character_test PRIVATE glad::glad)
if(APPLE)
    target_link_libraries(jolt_character_test PRIVATE "-framework OpenGL")
elseif(UNIX)
    target_link_libraries(jolt_character_test PRIVATE GL)
elseif(WIN32)
    target_link_libraries(jolt_character_test PRIVATE opengl32)
endif()
auto_install_target(jolt_character_test ${BINARY_ROOT_DIR})

# 重构后的版本
# ImGui 源文件
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/imgui)
set(IMGUI_BACKEND_SOURCES
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(jolt_character_test_new 
    jolt_test/jolt_character_test_new.cpp
    jolt_test/jolt_character_test_app.cpp
    jolt_test/physics_manager.cpp
    jolt_test/terrain_system.cpp
    jolt_test/character_controller.cpp
    jolt_test/camera.cpp
    jolt_test/input_manager.cpp
    jolt_test/shader_manager.cpp
    jolt_test/renderer.cpp
    ${IMGUI_BACKEND_SOURCES}
)
target_link_libraries(jolt_character_test_new PRIVATE glfw)
target_link_libraries(jolt_character_test_new PRIVATE Jolt::Jolt)
target_link_libraries(jolt_character_test_new PRIVATE glm::glm)
target_link_libraries(jolt_character_test_new PRIVATE glad::glad)
target_link_libraries(jolt_character_test_new PRIVATE imgui)
if(APPLE)
    target_link_libraries(jolt_character_test_new PRIVATE "-framework OpenGL")
elseif(UNIX)
    target_link_libraries(jolt_character_test_new PRIVATE GL)
elseif(WIN32)
    target_link_libraries(jolt_character_test_new PRIVATE opengl32)
endif()
auto_install_target(jolt_character_test_new ${BINARY_ROOT_DIR})

# Vulkan renderer test (refactored with separate Renderer, AssetManager, SceneManager)
add_executable(vulkan_renderer_test 
    vulkan_test/vulkan_render_test.cpp
    vulkan_test/renderer.cpp
    vulkan_test/asset_manager.cpp
    vulkan_test/scene_manager.cpp
    vulkan_test/input_manager.cpp
)
target_link_libraries(vulkan_renderer_test PRIVATE glfw)
target_link_libraries(vulkan_renderer_test PRIVATE glm::glm)
target_link_libraries(vulkan_renderer_test PRIVATE volk::volk)
target_link_libraries(vulkan_renderer_test PRIVATE tinyobjloader::tinyobjloader)
if(APPLE)
    # On macOS, we need to link against MoltenVK
    target_link_libraries(vulkan_renderer_test PRIVATE "-framework Metal")
    target_link_libraries(vulkan_renderer_test PRIVATE "-framework MetalKit")
    target_link_libraries(vulkan_renderer_test PRIVATE "-framework Cocoa")
    target_link_libraries(vulkan_renderer_test PRIVATE "-framework IOKit")
    target_link_libraries(vulkan_renderer_test PRIVATE "-framework CoreFoundation")
endif()
target_include_directories(vulkan_renderer_test PRIVATE ${TINYGLTF_INCLUDE_DIRS})
auto_install_target(vulkan_renderer_test ${BINARY_ROOT_DIR})
