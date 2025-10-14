#pragma once

#define VK_NO_PROTOTYPES
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <string>

class SceneManager;

class InputManager
{
public:
    InputManager(GLFWwindow* window);
    ~InputManager();

    // Update input state
    void update(float deltaTime);

    // Set scene manager for camera control
    void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }

    // Mouse control settings
    void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
    void setCameraSpeed(float speed) { m_cameraSpeed = speed; }

private:
    GLFWwindow* m_window;
    SceneManager* m_sceneManager;

    // Mouse state
    bool m_firstMouse = true;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    float m_mouseSensitivity = 0.1f;

    // Camera control
    float m_cameraSpeed = 2.5f;
    float m_yaw = -90.0f;   // Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right
    float m_pitch = 0.0f;

    // Input callbacks
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // Helper methods
    void processMouseMovement(double xpos, double ypos);
    void processScroll(double yoffset);
    void processKeyboard(float deltaTime);
};
