include(${PICCOLO_ROOT_DIR}/cmake/install.cmake)

add_executable(taskflow_test taskflow_test.cpp)

target_link_libraries(taskflow_test PRIVATE Taskflow::Taskflow)

auto_install_target(taskflow_test ${BINARY_ROOT_DIR}/test)
