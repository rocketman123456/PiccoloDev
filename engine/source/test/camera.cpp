#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace std;

Camera::Camera()
    : m_position(0, 5, 10)
    , m_target(0, 1, 0)
    , m_yaw(-90.0f)
    , m_pitch(-20.0f)
{
}

Camera::~Camera()
{
}

void Camera::Initialize()
{
    cout << "摄像机系统初始化完成" << endl;
}

void Camera::Update(const RVec3& character_position)
{
    // 更新摄像机跟随角色
    m_target = glm::vec3((float)character_position.GetX(), (float)character_position.GetY() + 1.0f, (float)character_position.GetZ());
    
    // 更新摄像机位置
    UpdateCameraPosition();
}

void Camera::HandleMouseMovement(double xpos, double ypos)
{
    if (!m_input_state.mouse_captured)
        return;

    if (m_input_state.first_mouse)
    {
        m_input_state.last_mouse_x = xpos;
        m_input_state.last_mouse_y = ypos;
        m_input_state.first_mouse = false;
        return;
    }

    float xoffset = xpos - m_input_state.last_mouse_x;
    float yoffset = m_input_state.last_mouse_y - ypos; // 反转 Y 轴
    m_input_state.last_mouse_x = xpos;
    m_input_state.last_mouse_y = ypos;

    xoffset *= m_config.sensitivity;
    yoffset *= m_config.sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    // 限制俯仰角范围
    ConstrainPitch();
}

void Camera::HandleScroll(double yoffset)
{
    m_config.distance -= (float)yoffset;
    if (m_config.distance < m_config.min_distance)
        m_config.distance = m_config.min_distance;
    if (m_config.distance > m_config.max_distance)
        m_config.distance = m_config.max_distance;
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(m_position, m_target, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::GetProjectionMatrix(float aspect_ratio) const
{
    return glm::perspective(glm::radians(m_config.fov), aspect_ratio, m_config.near_plane, m_config.far_plane);
}

void Camera::SetDistance(float distance)
{
    m_config.distance = distance;
    if (m_config.distance < m_config.min_distance)
        m_config.distance = m_config.min_distance;
    if (m_config.distance > m_config.max_distance)
        m_config.distance = m_config.max_distance;
}

void Camera::UpdateCameraPosition()
{
    // 根据偏航角和俯仰角计算摄像机位置
    float yaw_rad = glm::radians(m_yaw);
    float pitch_rad = glm::radians(m_pitch);
    
    glm::vec3 camera_offset;
    camera_offset.x = m_config.distance * cos(pitch_rad) * cos(yaw_rad);
    camera_offset.y = m_config.distance * sin(pitch_rad);
    camera_offset.z = m_config.distance * cos(pitch_rad) * sin(yaw_rad);

    m_position = m_target - camera_offset;
}

void Camera::ConstrainPitch()
{
    if (m_pitch > m_config.max_pitch)
        m_pitch = m_config.max_pitch;
    if (m_pitch < m_config.min_pitch)
        m_pitch = m_config.min_pitch;
}
