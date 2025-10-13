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
    jolt_character_test_new.cpp
    jolt_character_test_app.cpp
    physics_manager.cpp
    terrain_system.cpp
    character_controller.cpp
    camera.cpp
    input_manager.cpp
    shader_manager.cpp
    renderer.cpp
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
