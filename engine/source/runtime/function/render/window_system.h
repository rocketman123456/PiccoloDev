#pragma once

// #define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
// #include <volk.h>
#include <GLFW/glfw3.h>

#include <array>

namespace Piccolo
{
    struct WindowCreateInfo
    {
        int         width {1280};
        int         height {720};
        const char* title {"Piccolo"};
        bool        is_fullscreen {false};
    };

    class WindowSystem
    {
    public:
        WindowSystem() = default;
        ~WindowSystem();

        void initialize(WindowCreateInfo create_info);
        void pollEvents() const;
        bool shouldClose() const;
        void setTitle(const char* title);

        GLFWwindow*        getWindow() const;
        std::array<int, 2> getWindowSize() const;

    private:
        GLFWwindow* m_window {nullptr};
        int         m_width {0};
        int         m_height {0};

        bool m_is_focus_mode {false};
    };
} // namespace Piccolo