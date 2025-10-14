#include "input_manager.h"
#include "scene_manager.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
    // Map each GLFWwindow to its InputManager instance so we don't clobber the
    // window's existing user pointer (used by VulkanApplication).
    static std::unordered_map<GLFWwindow*, InputManager*> g_windowToInputManager;
}

InputManager::InputManager(GLFWwindow* window)
    : m_window(window)
    , m_sceneManager(nullptr)
{
    // Set up GLFW callbacks
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    
    // Register this window with our static map (do not touch the window user pointer)
    g_windowToInputManager[m_window] = this;
    
    // Capture mouse cursor
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

InputManager::~InputManager()
{
    // Restore cursor
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Unregister from the static map
    g_windowToInputManager.erase(m_window);
}

void InputManager::update(float deltaTime)
{
    if (!m_sceneManager)
        return;

    processKeyboard(deltaTime);
}

void InputManager::processMouseMovement(double xpos, double ypos)
{
    if (!m_sceneManager)
        return;

    if (m_firstMouse)
    {
        // Initialize yaw/pitch from current camera direction to avoid jump on first move
        auto& camera = m_sceneManager->getCamera();
        glm::vec3 currentDir = glm::normalize(camera.target - camera.position);
        m_pitch = glm::degrees(asinf(glm::clamp(currentDir.z, -1.0f, 1.0f))); // Z-up
        m_yaw = glm::degrees(atan2f(currentDir.y, currentDir.x));             // yaw about Z
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = false;
    }

    double xoffset = m_lastMouseX - xpos;
    double yoffset = m_lastMouseY - ypos; // Reversed since y-coordinates go from bottom to top
    m_lastMouseX = xpos;
    m_lastMouseY = ypos;

    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += static_cast<float>(xoffset);
    m_pitch += static_cast<float>(yoffset);

    // Constrain pitch to prevent screen flipping
    m_pitch = std::max(-89.0f, std::min(89.0f, m_pitch));

    // Update camera direction (Z-up convention)
    auto& camera = m_sceneManager->getCamera();
    
    // For Z-up: horizontal (X,Y) uses yaw; vertical (Z) uses pitch
    glm::vec3 direction;
    direction.x = cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
    direction.y = cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));
    direction.z = sin(glm::radians(m_pitch));
    
    // Update camera target (position + direction)
    camera.target = camera.position + glm::normalize(direction);
}

void InputManager::processScroll(double yoffset)
{
    if (!m_sceneManager)
        return;

    auto& camera = m_sceneManager->getCamera();
    
    // Adjust FOV based on scroll
    camera.fov -= static_cast<float>(yoffset);
    camera.fov = std::max(1.0f, std::min(90.0f, camera.fov));
}

void InputManager::processKeyboard(float deltaTime)
{
    if (!m_sceneManager)
        return;

    auto& camera = m_sceneManager->getCamera();
    float velocity = m_cameraSpeed * deltaTime;

    // Calculate camera direction vectors (Z-up/world-up movement)
    glm::vec3 direction = glm::normalize(camera.target - camera.position);
    glm::vec3 worldUp = camera.up; // expected (0,0,1)
    
    // Keep movement planar on X-Y by projecting forward onto ground plane
    glm::vec3 forward = glm::normalize(glm::vec3(direction.x, direction.y, 0.0f));
    if (glm::any(glm::isnan(forward)) || glm::length(forward) < 1e-4f)
    {
        forward = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
    glm::vec3 up = worldUp;

    // WASD movement
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.position += direction * velocity;
        camera.target += direction * velocity;
    }
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera.position -= direction * velocity;
        camera.target -= direction * velocity;
    }
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera.position -= right * velocity;
        camera.target -= right * velocity;
    }
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera.position += right * velocity;
        camera.target += right * velocity;
    }
    
    // Space and Shift for up/down movement
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        camera.position += up * velocity;
        camera.target += up * velocity;
    }
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        camera.position -= up * velocity;
        camera.target -= up * velocity;
    }
}

// Static callback functions
void InputManager::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto it = g_windowToInputManager.find(window);
    if (it != g_windowToInputManager.end() && it->second)
    {
        it->second->processMouseMovement(xpos, ypos);
    }
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto it = g_windowToInputManager.find(window);
    if (it != g_windowToInputManager.end() && it->second)
    {
        it->second->processScroll(yoffset);
    }
}

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Handle ESC key to exit
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}
