include(${PICCOLO_ROOT_DIR}/cmake/install.cmake)

add_executable(taskflow_test taskflow_test.cpp)
target_link_libraries(taskflow_test PRIVATE Taskflow::Taskflow)
auto_install_target(taskflow_test ${BINARY_ROOT_DIR})

# add_executable(shader_compile_test shader_compile_test.cpp)
# target_link_libraries(shader_compile_test PRIVATE PiccoloRuntime)
# auto_install_target(shader_compile_test ${BINARY_ROOT_DIR})

add_executable(reflection_test reflection_test.cpp)
target_link_libraries(reflection_test PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(reflection_test PRIVATE EnTT::EnTT)
target_include_directories(reflection_test PRIVATE ${REFL_CPP_INCLUDE_DIRS})
auto_install_target(reflection_test ${BINARY_ROOT_DIR})

add_executable(reflection_test_2 reflection_test_2.cpp)
target_link_libraries(reflection_test_2 PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(reflection_test_2 PRIVATE EnTT::EnTT)
target_include_directories(reflection_test_2 PRIVATE ${REFL_CPP_INCLUDE_DIRS})
target_link_libraries(reflection_test_2 PRIVATE reflectcpp::reflectcpp)
auto_install_target(reflection_test_2 ${BINARY_ROOT_DIR})
