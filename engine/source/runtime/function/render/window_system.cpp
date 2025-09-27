#include "runtime/function/render/window_system.h"

namespace Piccolo
{
    WindowSystem::~WindowSystem()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void WindowSystem::initialize(WindowCreateInfo create_info)
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(create_info.width, create_info.height, create_info.title, nullptr, nullptr);
    }

    void WindowSystem::pollEvents() const { glfwPollEvents(); }

    bool WindowSystem::shouldClose() const { return glfwWindowShouldClose(m_window); }

    void WindowSystem::setTitle(const char* title) { glfwSetWindowTitle(m_window, title); }

    GLFWwindow* WindowSystem::getWindow() const { return m_window; }

    std::array<int, 2> WindowSystem::getWindowSize() const { return std::array<int, 2>({m_width, m_height}); }

} // namespace Piccolo