#include "runtime/function/render/window_system.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_system.h"

namespace Piccolo
{
    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
    {
        //
        g_runtime_global_context.m_render_system->setFramebufferResized(true);
    }

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

        glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);

        if (!m_window)
        {
            LOG_ERROR("Failed to create GLFW window");
        }
    }

    void WindowSystem::pollEvents() const { glfwPollEvents(); }

    bool WindowSystem::shouldClose() const { return glfwWindowShouldClose(m_window); }

    void WindowSystem::setTitle(const char* title) { glfwSetWindowTitle(m_window, title); }

    GLFWwindow* WindowSystem::getWindow() const { return m_window; }

    std::array<int, 2> WindowSystem::getWindowSize() const { return std::array<int, 2>({m_width, m_height}); }

} // namespace Piccolo
