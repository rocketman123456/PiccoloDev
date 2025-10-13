#include "input_manager.h"
#include "terrain_system.h"
#include "physics_manager.h"
#include <iostream>

using namespace std;

// 静态成员定义
InputManager* InputManager::s_instance = nullptr;

InputManager::InputManager()
    : m_window(nullptr)
    , m_camera(nullptr)
    , m_terrain_system(nullptr)
    , m_physics_manager(nullptr)
{
    s_instance = this;
}

InputManager::~InputManager()
{
    Shutdown();
    s_instance = nullptr;
}

void InputManager::Initialize(GLFWwindow* window)
{
    m_window = window;
    
    // 设置回调函数
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    
    cout << "输入管理器初始化成功" << endl;
}

void InputManager::Update()
{
    // 目前不需要每帧更新
}

void InputManager::Shutdown()
{
    // 清理资源
    m_window = nullptr;
    m_camera = nullptr;
    m_terrain_system = nullptr;
    m_physics_manager = nullptr;
}

void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (!s_instance)
        return;
    
    s_instance->HandleKeyPress(key, action);
}

void InputManager::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!s_instance || !s_instance->m_input_state.mouse_captured || !s_instance->m_camera)
        return;
    
    if (s_instance->m_input_state.first_mouse)
    {
        s_instance->m_input_state.last_mouse_x = xpos;
        s_instance->m_input_state.last_mouse_y = ypos;
        s_instance->m_input_state.first_mouse = false;
    }
    
    double xoffset = xpos - s_instance->m_input_state.last_mouse_x;
    double yoffset = s_instance->m_input_state.last_mouse_y - ypos; // 反转y轴
    
    s_instance->m_input_state.last_mouse_x = xpos;
    s_instance->m_input_state.last_mouse_y = ypos;
    
    // 更新摄像机偏航和俯仰角
    float yaw_offset = static_cast<float>(xoffset) * 0.1f; // sensitivity
    float pitch_offset = static_cast<float>(yoffset) * 0.1f;
    
    s_instance->m_camera->SetYaw(s_instance->m_camera->GetYaw() + yaw_offset);
    s_instance->m_camera->SetPitch(s_instance->m_camera->GetPitch() + pitch_offset);
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!s_instance || !s_instance->m_camera)
        return;
    
    // 调整摄像机距离
    float new_distance = s_instance->m_camera->GetDistance() - static_cast<float>(yoffset);
    s_instance->m_camera->SetDistance(new_distance);
}

void InputManager::HandleKeyPress(int key, int action)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        if (m_window)
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
    
    bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
    
    // 移动键
    if (key == GLFW_KEY_W)
        m_input_state.key_w = pressed;
    if (key == GLFW_KEY_S)
        m_input_state.key_s = pressed;
    if (key == GLFW_KEY_A)
        m_input_state.key_a = pressed;
    if (key == GLFW_KEY_D)
        m_input_state.key_d = pressed;
    if (key == GLFW_KEY_SPACE)
        m_input_state.key_space = pressed;
    
    // 特殊键（只在按下时触发）
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_P)
            ToggleMouseCapture();
        if (key == GLFW_KEY_O)
            ToggleDebugCollisionMesh();
        if (key == GLFW_KEY_B)
            ToggleDebugGroundCollisionSolid();
        if (key == GLFW_KEY_C)
            ToggleDebugCharacterCollision();
    }
}

void InputManager::ToggleMouseCapture()
{
    m_input_state.mouse_captured = !m_input_state.mouse_captured;
    
    if (m_window)
    {
        if (m_input_state.mouse_captured)
        {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_input_state.first_mouse = true;
            cout << "鼠标已捕获 - 可以旋转摄像机" << endl;
        }
        else
        {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cout << "鼠标已释放" << endl;
        }
    }
}

void InputManager::ToggleDebugCollisionMesh()
{
    m_debug_options.debug_collision_mesh = !m_debug_options.debug_collision_mesh;
    cout << "调试碰撞网格: " << (m_debug_options.debug_collision_mesh ? "开启" : "关闭") << endl;
}

void InputManager::ToggleDebugGroundCollisionSolid()
{
    m_debug_options.debug_ground_collision_solid = !m_debug_options.debug_ground_collision_solid;
    cout << "地面碰撞体实心渲染: " << (m_debug_options.debug_ground_collision_solid ? "开启" : "关闭") << endl;
}

void InputManager::ToggleDebugCharacterCollision()
{
    m_debug_options.debug_character_collision = !m_debug_options.debug_character_collision;
    cout << "角色碰撞体调试: " << (m_debug_options.debug_character_collision ? "开启" : "关闭") << endl;
}
