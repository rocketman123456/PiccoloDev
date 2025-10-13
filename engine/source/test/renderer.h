#pragma once

#include "jolt_character_test_common.h"
#include "camera.h"
#include "terrain_system.h"
#include "character_controller.h"

// 渲染器
class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Render(const Camera& camera, const TerrainSystem& terrain_system, 
                const CharacterController& character_controller, const PhysicsManager& physics_manager,
                const DebugOptions& debug_options, float aspect_ratio);
    void Shutdown();

    // 几何体创建
    void CreateCapsuleGeometry(float radius, float half_height);
    void CreateDebugCollisionGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies, 
                                     const std::map<std::pair<int, int>, float>& terrain_data);
    void CreateCharacterDebugGeometry();
    void CreateGroundCollisionSolidGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies,
                                           const std::map<std::pair<int, int>, float>& terrain_data);

    // 几何体更新
    void UpdateDebugCollisionGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies,
                                     const std::map<std::pair<int, int>, float>& terrain_data);
    void UpdateGroundCollisionSolidGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies,
                                           const std::map<std::pair<int, int>, float>& terrain_data);

    // OpenGL资源访问
    GLuint GetCapsuleVAO() const { return m_capsule_vao; }
    int GetCapsuleVertexCount() const { return m_capsule_vertex_count; }
    GLuint GetDebugVAO() const { return m_debug_vao; }
    int GetDebugVertexCount() const { return m_debug_vertex_count; }
    GLuint GetCharacterDebugVAO() const { return m_character_debug_vao; }
    int GetCharacterDebugVertexCount() const { return m_character_debug_vertex_count; }
    GLuint GetGroundCollisionVAO() const { return m_ground_collision_vao; }
    int GetGroundCollisionVertexCount() const { return m_ground_collision_vertex_count; }

private:
    ShaderManager* m_shader_manager;
    
    // 胶囊体几何体
    GLuint m_capsule_vao;
    GLuint m_capsule_vbo;
    int m_capsule_vertex_count;
    
    // 调试碰撞网格
    GLuint m_debug_vao;
    GLuint m_debug_vbo;
    int m_debug_vertex_count;
    
    // 角色碰撞体调试
    GLuint m_character_debug_vao;
    GLuint m_character_debug_vbo;
    int m_character_debug_vertex_count;
    
    // 地面碰撞体实心
    GLuint m_ground_collision_vao;
    GLuint m_ground_collision_vbo;
    int m_ground_collision_vertex_count;

    // 内部方法
    void SetupOpenGLState();
    void RenderTerrain(const Camera& camera, const TerrainSystem& terrain_system);
    void RenderCharacter(const Camera& camera, const CharacterController& character_controller);
    void RenderDebugCollisionMesh(const Camera& camera);
    void RenderGroundCollisionSolid(const Camera& camera, const TerrainSystem& terrain_system);
    void RenderCharacterDebug(const Camera& camera, const CharacterController& character_controller);
    
    void CleanupOpenGLResources();
    void AddQuadUltraFast(std::vector<float>& vertices, float x1, float y1, float z1, float x2, float y2, float z2, 
                         float x3, float y3, float z3, float x4, float y4, float z4, float nx, float ny, float nz);
    void CreateCollisionCheckerboardTexture(GLuint& texture_id);
};
