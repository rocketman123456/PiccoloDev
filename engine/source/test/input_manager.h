#pragma once

#include "jolt_character_test_common.h"

// 前向声明
class TerrainSystem;
class PhysicsManager;
class Camera;

// 输入管理器
class InputManager
{
public:
    InputManager();
    ~InputManager();

    void Initialize(GLFWwindow* window);
    void Update();
    void Shutdown();

    // 输入状态访问
    const InputState& GetInputState() const { return m_input_state; }
    InputState& GetInputState() { return m_input_state; }

    // 键盘回调
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    
    // 鼠标回调
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // 调试选项访问
    const DebugOptions& GetDebugOptions() const { return m_debug_options; }
    DebugOptions& GetDebugOptions() { return m_debug_options; }

    // 设置回调目标
    void SetCamera(Camera* camera) { m_camera = camera; }
    void SetTerrainSystem(TerrainSystem* terrain_system) { m_terrain_system = terrain_system; }
    void SetPhysicsManager(PhysicsManager* physics_manager) { m_physics_manager = physics_manager; }

private:
    GLFWwindow* m_window;
    InputState m_input_state;
    DebugOptions m_debug_options;
    
    // 回调目标
    Camera* m_camera;
    TerrainSystem* m_terrain_system;
    PhysicsManager* m_physics_manager;

    // 静态实例指针（用于回调）
    static InputManager* s_instance;

    // 内部方法
    void HandleKeyPress(int key, int action);
    void ToggleMouseCapture();
    void ToggleDebugCollisionMesh();
    void ToggleDebugGroundCollisionSolid();
    void ToggleDebugCharacterCollision();
};
