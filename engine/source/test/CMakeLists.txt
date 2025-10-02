include(${PICCOLO_ROOT_DIR}/cmake/install.cmake)

add_executable(taskflow_test taskflow_test.cpp)
target_link_libraries(taskflow_test PRIVATE Taskflow::Taskflow)
auto_install_target(taskflow_test ${BINARY_ROOT_DIR}/test)

add_executable(shader_compile_test shader_compile_test.cpp)
target_link_libraries(shader_compile_test PRIVATE PiccoloRuntime)
auto_install_target(shader_compile_test ${BINARY_ROOT_DIR})
