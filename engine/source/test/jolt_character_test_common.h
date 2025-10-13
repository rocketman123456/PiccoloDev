#pragma once

// clang-format off
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/Character.h>
// clang-format on

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <cmath>
#include <random>
#include <thread>

using namespace JPH;

// 层定义
namespace Layers
{
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING     = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
}

// 宽相层定义
namespace BPLayers
{
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint            NUM_LAYERS(2);
}

// 地形参数
struct TerrainConfig
{
    int   grid_size = 50;      // 地形网格大小 (减少到50x50=2500个块)
    float tile_size = 1.0f;    // 每个瓦片的大小
    int   physics_radius = 5;  // 物理系统半径
    int   terrain_radius = 20; // 地形生成半径
    int   update_threshold = 10; // 更新阈值
};

// 角色配置
struct CharacterConfig
{
    float radius = 0.3f;
    float half_height = 0.4f;
    float move_speed = 5.0f;
    float jump_speed = 4.0f;
    float max_slope_angle = 45.0f;
    float friction = 0.5f;
};

// 摄像机配置
struct CameraConfig
{
    float fov = 45.0f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    float distance = 5.0f;
    float min_distance = 2.0f;
    float max_distance = 50.0f;
    float sensitivity = 0.1f;
    float min_pitch = -89.0f;
    float max_pitch = 89.0f;
};

// 输入状态
struct InputState
{
    bool key_w = false;
    bool key_s = false;
    bool key_a = false;
    bool key_d = false;
    bool key_space = false;
    bool mouse_captured = false;
    double last_mouse_x = 400.0;
    double last_mouse_y = 300.0;
    bool first_mouse = true;
};

// 调试选项
struct DebugOptions
{
    bool debug_collision_mesh = false;
    bool debug_ground_collision_solid = false;
    bool debug_character_collision = false;
};

// 前向声明
class PhysicsManager;
class TerrainSystem;
class Renderer;
class InputManager;
class ShaderManager;
