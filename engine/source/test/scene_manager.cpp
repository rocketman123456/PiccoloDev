#include "scene_manager.h"
#include "asset_manager.h"

#include <algorithm>

// Transform implementation
glm::mat4 Transform::getMatrix() const
{
    glm::mat4 mat = glm::mat4(1.0f);
    mat = glm::translate(mat, position);
    mat = glm::rotate(mat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::scale(mat, scale);
    return mat;
}

// Camera implementation
glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const
{
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    proj[1][1] *= -1; // Flip Y for Vulkan
    return proj;
}

// SceneManager implementation
SceneManager::SceneManager()
{
}

SceneManager::~SceneManager()
{
}

SceneObject* SceneManager::createObject(const std::string& name)
{
    auto object = std::make_shared<SceneObject>();
    object->name = name;
    m_objects.push_back(object);
    return object.get();
}

SceneObject* SceneManager::getObject(const std::string& name)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const std::shared_ptr<SceneObject>& obj) {
            return obj->name == name;
        });
    
    if (it != m_objects.end())
    {
        return it->get();
    }
    return nullptr;
}

void SceneManager::removeObject(const std::string& name)
{
    m_objects.erase(
        std::remove_if(m_objects.begin(), m_objects.end(),
            [&name](const std::shared_ptr<SceneObject>& obj) {
                return obj->name == name;
            }),
        m_objects.end()
    );
}

void SceneManager::update(float deltaTime)
{
    m_time += deltaTime;

    // Update objects (e.g., animations, rotations)
    for (auto& object : m_objects)
    {
        if (!object->enabled)
            continue;

        // Example: rotate objects over time
        // object->transform.rotation.z = m_time * glm::radians(90.0f);
    }
}

UniformBufferObject SceneManager::getUBO(const SceneObject& object, float aspectRatio) const
{
    UniformBufferObject ubo{};
    ubo.model = object.transform.getMatrix();
    ubo.view = m_camera.getViewMatrix();
    ubo.proj = m_camera.getProjectionMatrix(aspectRatio);
    return ubo;
}

