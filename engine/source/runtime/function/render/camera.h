#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Piccolo
{
    class RenderCamera
    {
    public:
        RenderCamera()
        {
            m_position = glm::vec3(0.0f, 2.0f, 6.0f);
            m_yaw      = -90.0f;
            m_pitch    = -10.0f;
            m_fov      = 45.0f;
            m_znear    = 0.1f;
            m_zfar     = 500.0f;
        }

        void setAspect(float aspect) { m_aspect = aspect; }

        glm::mat4 getView() const
        {
            glm::vec3 front;
            front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            front.y = sin(glm::radians(m_pitch));
            front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            glm::vec3 direction = glm::normalize(front);
            return glm::lookAt(m_position, m_position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::mat4 getProj() const
        {
            glm::mat4 proj = glm::perspective(glm::radians(m_fov), m_aspect, m_znear, m_zfar);
            // Vulkan clip space has inverted Y
            proj[1][1] *= -1.0f;
            return proj;
        }

        void move(const glm::vec3& delta)
        {
            glm::vec3 front;
            front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            front.y = sin(glm::radians(m_pitch));
            front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            glm::vec3 forward = glm::normalize(front);
            glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 up      = glm::vec3(0.0f, 1.0f, 0.0f);

            m_position += forward * delta.z + right * delta.x + up * delta.y;
        }

        void rotate(float yaw_delta_deg, float pitch_delta_deg)
        {
            m_yaw += yaw_delta_deg;
            m_pitch += pitch_delta_deg;
            if (m_pitch > 89.0f) m_pitch = 89.0f;
            if (m_pitch < -89.0f) m_pitch = -89.0f;
        }

        const glm::vec3& getPosition() const { return m_position; }
        float getYaw() const { return m_yaw; }
        float getPitch() const { return m_pitch; }

    private:
        glm::vec3 m_position;
        float     m_yaw;
        float     m_pitch;
        float     m_fov;
        float     m_znear;
        float     m_zfar;
        float     m_aspect {16.0f / 9.0f};
    };
} // namespace Piccolo


