#include "runtime/function/input/input_manager.h"

#include "runtime/function/render/camera.h"
#include "runtime/function/render/window_system.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Piccolo
{
    void InputManager::initialize(WindowSystem* window_system, RenderCamera* camera)
    {
        m_window_system = window_system;
        m_camera        = camera;

        // Initialize mouse position
        double x, y;
        glfwGetCursorPos(m_window_system->getWindow(), &x, &y);
        m_last_x      = x;
        m_last_y      = y;
        m_first_mouse = false;

        // Capture mouse
        glfwSetInputMode(m_window_system->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_mouse_captured = true;
    }

    void InputManager::tick(float dt)
    {
        if (!m_window_system || !m_camera)
            return;

        // Toggle mouse capture with 'P'
        GLFWwindow* window = m_window_system->getWindow();
        int         p_state = glfwGetKey(window, GLFW_KEY_P);
        bool        p_pressed = (p_state == GLFW_PRESS);
        if (p_pressed && !m_prev_p_pressed)
        {
            m_mouse_captured = !m_mouse_captured;
            glfwSetInputMode(window, GLFW_CURSOR, m_mouse_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            m_first_mouse = true; // reset so next movement delta is zeroed
        }
        m_prev_p_pressed = p_pressed;

        if (m_mouse_captured)
        {
            updateMouse(dt);
        }
        updateKeyboard(dt);

        // Update camera aspect based on window size
        auto size = m_window_system->getWindowSize();
        if (size[1] > 0)
        {
            m_camera->setAspect(static_cast<float>(size[0]) / static_cast<float>(size[1]));
        }
    }

    void InputManager::updateMouse(float /*dt*/)
    {
        GLFWwindow* window = m_window_system->getWindow();
        double      x, y;
        glfwGetCursorPos(window, &x, &y);

        if (m_first_mouse)
        {
            m_last_x      = x;
            m_last_y      = y;
            m_first_mouse = false;
            return;
        }

        double xoffset = x - m_last_x;
        double yoffset = m_last_y - y; // reversed since y-coordinates go from bottom to top
        m_last_x       = x;
        m_last_y       = y;

        float yaw_delta   = static_cast<float>(xoffset) * m_mouse_sensitivity;
        float pitch_delta = static_cast<float>(yoffset) * m_mouse_sensitivity;
        m_camera->rotate(yaw_delta, pitch_delta);
    }

    void InputManager::updateKeyboard(float dt)
    {
        GLFWwindow* window = m_window_system->getWindow();

        glm::vec3 move_delta(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            move_delta.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            move_delta.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            move_delta.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            move_delta.x += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            move_delta.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            move_delta.y -= 1.0f;

        if (glm::length(move_delta) > 0.0f)
        {
            move_delta = glm::normalize(move_delta);
        }

        m_camera->move(move_delta * m_move_speed * dt);
    }
} // namespace Piccolo


