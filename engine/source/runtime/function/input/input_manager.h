#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

namespace Piccolo
{
    class RenderCamera;
    class WindowSystem;

    class InputManager
    {
    public:
        void initialize(WindowSystem* window_system, RenderCamera* camera);
        void tick(float dt);

    private:
        void updateMouse(float dt);
        void updateKeyboard(float dt);

        WindowSystem* m_window_system {nullptr};
        RenderCamera* m_camera {nullptr};

        bool  m_first_mouse {true};
        double m_last_x {0.0};
        double m_last_y {0.0};

        float m_mouse_sensitivity {0.12f};
        float m_move_speed {6.0f};

        bool  m_mouse_captured {true};
        bool  m_prev_p_pressed {false};
    };
} // namespace Piccolo


