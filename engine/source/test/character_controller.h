#pragma once

#include "jolt_character_test_common.h"
#include "physics_manager.h"
#include <Jolt/Physics/Character/Character.h>

// 角色控制器
class CharacterController
{
public:
    CharacterController();
    ~CharacterController();

    bool Initialize(PhysicsManager* physics_manager);
    void Update(float delta_time, const InputState& input_state, const glm::vec3& camera_forward, const glm::vec3& camera_right);
    void Shutdown();

    // 角色状态
    RVec3 GetPosition() const;
    Vec3 GetVelocity() const;
    Character::EGroundState GetGroundState() const;

    // 移动控制
    void SetMoveSpeed(float speed) { m_config.move_speed = speed; }
    void SetJumpSpeed(float speed) { m_config.jump_speed = speed; }

private:
    PhysicsManager* m_physics_manager;
    CharacterConfig m_config;
    
    // 内部方法
    Vec3 CalculateMovementDirection(const InputState& input_state, const glm::vec3& camera_forward, const glm::vec3& camera_right) const;
    void HandleJump(const InputState& input_state, Vec3& desired_velocity) const;
};
