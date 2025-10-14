#pragma once

#include "jolt_character_test_common.h"
#include "physics_manager.h"
#include "terrain_system.h"
#include "character_controller.h"
#include "camera.h"
#include "input_manager.h"
#include "shader_manager.h"
#include "renderer.h"
#include <GLFW/glfw3.h>

// 主应用程序类
class JoltCharacterTestApp
{
public:
    JoltCharacterTestApp();
    ~JoltCharacterTestApp();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    GLFWwindow* m_window;
    
    // 系统组件
    PhysicsManager* m_physics_manager;
    TerrainSystem* m_terrain_system;
    CharacterController* m_character_controller;
    Camera* m_camera;
    InputManager* m_input_manager;
    ShaderManager* m_shader_manager;
    Renderer* m_renderer;

    // 应用程序状态
    bool m_initialized;
    bool m_imgui_initialized;
    double m_last_time;
    
    // 窗口状态
    int m_window_width;
    int m_window_height;
    int m_framebuffer_width;
    int m_framebuffer_height;

    // 内部方法
    bool InitializeGLFW();
    bool InitializeOpenGL();
    bool InitializeSystems();
    bool InitializeImGui();
    void Update(float delta_time);
    void Render();
    void RenderImGui(float delta_time);
    void Cleanup();
    void CleanupImGui();
    
    // 窗口回调
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    void OnWindowResize(int width, int height);
    void OnFramebufferResize(int width, int height);
};
