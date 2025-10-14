#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <vector>
#include <string>

class AssetManager;
struct Mesh;
struct Texture;

struct Transform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in radians
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 getMatrix() const;
};

struct Camera
{
    glm::vec3 position = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 10.0f;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
};

struct SceneObject
{
    std::string name;
    Transform transform;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> texture;
    bool enabled = true;
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    // Scene object management
    SceneObject* createObject(const std::string& name);
    SceneObject* getObject(const std::string& name);
    void removeObject(const std::string& name);
    const std::vector<std::shared_ptr<SceneObject>>& getObjects() const { return m_objects; }

    // Camera management
    Camera& getCamera() { return m_camera; }
    const Camera& getCamera() const { return m_camera; }
    void setCamera(const Camera& camera) { m_camera = camera; }

    // Update
    void update(float deltaTime);

    // Get UBO for rendering
    UniformBufferObject getUBO(const SceneObject& object, float aspectRatio) const;

private:
    std::vector<std::shared_ptr<SceneObject>> m_objects;
    Camera m_camera;
    float m_time = 0.0f;
};

