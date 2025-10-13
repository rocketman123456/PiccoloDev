#pragma once

#include "jolt_character_test_common.h"

// 摄像机类
class Camera
{
public:
    Camera();
    ~Camera();

    void Initialize();
    void Update(const RVec3& character_position);
    void HandleMouseMovement(double xpos, double ypos);
    void HandleScroll(double yoffset);

    // 获取摄像机参数
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetTarget() const { return m_target; }
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect_ratio) const;
    
    // 摄像机控制
    void SetMouseCaptured(bool captured) { m_input_state.mouse_captured = captured; }
    bool IsMouseCaptured() const { return m_input_state.mouse_captured; }
    void SetFirstMouse(bool first) { m_input_state.first_mouse = first; }

    // 配置
    void SetDistance(float distance);
    void SetYaw(float yaw) { m_yaw = yaw; }
    void SetPitch(float pitch) { m_pitch = pitch; }
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetDistance() const { return m_config.distance; }
    float GetSensitivity() const { return m_config.sensitivity; }

private:
    glm::vec3 m_position;
    glm::vec3 m_target;
    float m_yaw;
    float m_pitch;
    
    CameraConfig m_config;
    InputState m_input_state; // 只使用鼠标相关部分

    // 内部方法
    void UpdateCameraPosition();
    void ConstrainPitch();
};
