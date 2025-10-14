#include "character_controller.h"
#include "physics_manager.h"
#include <iostream>

using namespace std;

CharacterController::CharacterController()
    : m_physics_manager(nullptr)
{
}

CharacterController::~CharacterController()
{
    Shutdown();
}

bool CharacterController::Initialize(PhysicsManager* physics_manager)
{
    if (!physics_manager)
    {
        cerr << "PhysicsManager 为空，无法初始化角色控制器" << endl;
        return false;
    }

    m_physics_manager = physics_manager;
    cout << "角色控制器初始化成功" << endl;
    return true;
}

void CharacterController::Update(float delta_time, const InputState& input_state, const glm::vec3& camera_forward, const glm::vec3& camera_right)
{
    if (!m_physics_manager)
        return;

    // 计算移动方向
    Vec3 movement_direction = CalculateMovementDirection(input_state, camera_forward, camera_right);

    // 设置角色速度
    Vec3 current_velocity = m_physics_manager->GetCharacterVelocity();
    Vec3 desired_velocity = movement_direction * m_config.move_speed;
    desired_velocity.SetY(current_velocity.GetY()); // 保持垂直速度

    // 处理跳跃
    HandleJump(input_state, desired_velocity);

    // 应用速度到物理系统
    m_physics_manager->SetCharacterVelocity(desired_velocity);
}

void CharacterController::Shutdown()
{
    m_physics_manager = nullptr;
}

RVec3 CharacterController::GetPosition() const
{
    if (m_physics_manager)
    {
        return m_physics_manager->GetCharacterPosition();
    }
    return RVec3::sZero();
}

Vec3 CharacterController::GetVelocity() const
{
    if (m_physics_manager)
    {
        return m_physics_manager->GetCharacterVelocity();
    }
    return Vec3::sZero();
}

Character::EGroundState CharacterController::GetGroundState() const
{
    if (m_physics_manager)
    {
        return m_physics_manager->GetCharacterGroundState();
    }
    return Character::EGroundState::InAir;
}

Vec3 CharacterController::CalculateMovementDirection(const InputState& input_state, const glm::vec3& camera_forward, const glm::vec3& camera_right) const
{
    // 将glm::vec3转换为Jolt Vec3
    Vec3 camera_forward_jolt(camera_forward.x, camera_forward.y, camera_forward.z);
    Vec3 camera_right_jolt(camera_right.x, camera_right.y, camera_right.z);

    // 根据输入计算相对于摄像机的移动方向
    Vec3 movement_direction(0, 0, 0);
    if (input_state.key_w)
        movement_direction += camera_forward_jolt;
    if (input_state.key_s)
        movement_direction -= camera_forward_jolt;
    if (input_state.key_a)
        movement_direction -= camera_right_jolt;
    if (input_state.key_d)
        movement_direction += camera_right_jolt;

    // 归一化移动方向
    float length = movement_direction.Length();
    if (length > 0.001f)
    {
        movement_direction = movement_direction / length;
    }

    return movement_direction;
}

void CharacterController::HandleJump(const InputState& input_state, Vec3& desired_velocity) const
{
    if (input_state.key_space && GetGroundState() == Character::EGroundState::OnGround)
    {
        desired_velocity.SetY(m_config.jump_speed); // 跳跃速度
    }
}
