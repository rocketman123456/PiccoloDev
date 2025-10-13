#include "renderer.h"
#include "shader_manager.h"
#include "physics_manager.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <iostream>
#include <cmath>

using namespace std;
using namespace JPH;

Renderer::Renderer()
    : m_shader_manager(nullptr)
    , m_capsule_vao(0)
    , m_capsule_vbo(0)
    , m_capsule_vertex_count(0)
    , m_debug_vao(0)
    , m_debug_vbo(0)
    , m_debug_vertex_count(0)
    , m_character_debug_vao(0)
    , m_character_debug_vbo(0)
    , m_character_debug_vertex_count(0)
    , m_ground_collision_vao(0)
    , m_ground_collision_vbo(0)
    , m_ground_collision_vertex_count(0)
{
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize()
{
    // 创建着色器管理器
    m_shader_manager = new ShaderManager();
    if (!m_shader_manager->Initialize())
    {
        cerr << "着色器管理器初始化失败" << endl;
        return false;
    }
    
    // 创建角色胶囊体几何体
    CreateCapsuleGeometry(0.3f, 0.4f); // 半径0.3m，半高度0.4m
    
    // 创建角色调试几何体
    CreateCharacterDebugGeometry();
    
    cout << "渲染器初始化成功" << endl;
    return true;
}

void Renderer::Render(const Camera& camera, const TerrainSystem& terrain_system, 
                      const CharacterController& character_controller, const PhysicsManager& physics_manager,
                      const DebugOptions& debug_options, float aspect_ratio)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 设置OpenGL状态
    SetupOpenGLState();

    // 使用着色器程序
    GLuint shader_program = m_shader_manager->GetShaderProgram();
    glUseProgram(shader_program);

    // 设置矩阵
    glm::mat4 projection = camera.GetProjectionMatrix(aspect_ratio);
    glm::mat4 view = camera.GetViewMatrix();

    GLint proj_loc = glGetUniformLocation(shader_program, "projection");
    GLint view_loc = glGetUniformLocation(shader_program, "view");
    GLint model_loc = glGetUniformLocation(shader_program, "model");
    GLint light_pos_loc = glGetUniformLocation(shader_program, "lightPos");
    GLint view_pos_loc = glGetUniformLocation(shader_program, "viewPos");
    GLint object_color_loc = glGetUniformLocation(shader_program, "objectColor");
    GLint use_texture_loc = glGetUniformLocation(shader_program, "useTexture");
    GLint use_alpha_loc = glGetUniformLocation(shader_program, "useAlpha");

    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniform3f(light_pos_loc, 10.0f, 20.0f, 10.0f);
    
    glm::vec3 cam_pos = camera.GetPosition();
    glUniform3f(view_pos_loc, cam_pos.x, cam_pos.y, cam_pos.z);

    // 渲染地形
    RenderTerrain(camera, terrain_system);

    // 渲染角色
    RenderCharacter(camera, character_controller);

    // 渲染调试碰撞网格（绿色线框）
    if (debug_options.debug_collision_mesh)
    {
        RenderDebugCollisionMesh(camera);
    }

    // 渲染地面碰撞体实心盒子（蓝色实心）
    if (debug_options.debug_ground_collision_solid)
    {
        RenderGroundCollisionSolid(camera, terrain_system);
    }

    // 渲染角色碰撞体调试网格（红色线框）
    if (debug_options.debug_character_collision)
    {
        RenderCharacterDebug(camera, character_controller);
    }

    glBindVertexArray(0);
}

void Renderer::Shutdown()
{
    CleanupOpenGLResources();
    
    if (m_shader_manager)
    {
        delete m_shader_manager;
        m_shader_manager = nullptr;
    }
}

void Renderer::CreateCapsuleGeometry(float radius, float half_height)
{
    std::vector<float> vertices;
    
    // 胶囊体由三部分组成：
    // 1. 圆柱体（中间部分）
    // 2. 上半球（顶部）
    // 3. 下半球（底部）
    
    int latitude_segments = 16;
    int longitude_segments = 32;
    
    // 圆柱体部分
    for (int i = 0; i < longitude_segments; ++i)
    {
        float theta1 = (float)i / longitude_segments * 2.0f * M_PI;
        float theta2 = (float)(i + 1) / longitude_segments * 2.0f * M_PI;
        
        float x1 = radius * cosf(theta1);
        float z1 = radius * sinf(theta1);
        float x2 = radius * cosf(theta2);
        float z2 = radius * sinf(theta2);
        
        // 法线
        float nx1 = cosf(theta1);
        float nz1 = sinf(theta1);
        float nx2 = cosf(theta2);
        float nz2 = sinf(theta2);
        
        // 添加两个三角形组成一个四边形
        // 三角形1
        vertices.insert(vertices.end(), {x1, -half_height, z1, nx1, 0.0f, nz1, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {x2, -half_height, z2, nx2, 0.0f, nz2, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {x2, half_height, z2, nx2, 0.0f, nz2, 1.0f, 1.0f});
        
        // 三角形2
        vertices.insert(vertices.end(), {x1, -half_height, z1, nx1, 0.0f, nz1, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {x2, half_height, z2, nx2, 0.0f, nz2, 1.0f, 1.0f});
        vertices.insert(vertices.end(), {x1, half_height, z1, nx1, 0.0f, nz1, 0.0f, 1.0f});
    }
    
    // 上半球
    for (int j = 0; j < latitude_segments / 2; ++j)
    {
        float phi1 = (float)j / latitude_segments * M_PI;
        float phi2 = (float)(j + 1) / latitude_segments * M_PI;
        
        for (int i = 0; i < longitude_segments; ++i)
        {
            float theta1 = (float)i / longitude_segments * 2.0f * M_PI;
            float theta2 = (float)(i + 1) / longitude_segments * 2.0f * M_PI;
            
            // 第一个三角形
            float x1 = radius * sinf(phi1) * cosf(theta1);
            float y1 = radius * cosf(phi1) + half_height;
            float z1 = radius * sinf(phi1) * sinf(theta1);
            
            float x2 = radius * sinf(phi1) * cosf(theta2);
            float y2 = radius * cosf(phi1) + half_height;
            float z2 = radius * sinf(phi1) * sinf(theta2);
            
            float x3 = radius * sinf(phi2) * cosf(theta2);
            float y3 = radius * cosf(phi2) + half_height;
            float z3 = radius * sinf(phi2) * sinf(theta2);
            
            // 法线（球面法线）
            float nx1 = sinf(phi1) * cosf(theta1);
            float ny1 = cosf(phi1);
            float nz1 = sinf(phi1) * sinf(theta1);
            
            float nx2 = sinf(phi1) * cosf(theta2);
            float ny2 = cosf(phi1);
            float nz2 = sinf(phi1) * sinf(theta2);
            
            float nx3 = sinf(phi2) * cosf(theta2);
            float ny3 = cosf(phi2);
            float nz3 = sinf(phi2) * sinf(theta2);
            
            vertices.insert(vertices.end(), {x1, y1, z1, nx1, ny1, nz1, 0.0f, 0.0f});
            vertices.insert(vertices.end(), {x2, y2, z2, nx2, ny2, nz2, 1.0f, 0.0f});
            vertices.insert(vertices.end(), {x3, y3, z3, nx3, ny3, nz3, 1.0f, 1.0f});
            
            // 第二个三角形
            float x4 = radius * sinf(phi2) * cosf(theta1);
            float y4 = radius * cosf(phi2) + half_height;
            float z4 = radius * sinf(phi2) * sinf(theta1);
            
            float nx4 = sinf(phi2) * cosf(theta1);
            float ny4 = cosf(phi2);
            float nz4 = sinf(phi2) * sinf(theta1);
            
            vertices.insert(vertices.end(), {x1, y1, z1, nx1, ny1, nz1, 0.0f, 0.0f});
            vertices.insert(vertices.end(), {x3, y3, z3, nx3, ny3, nz3, 1.0f, 1.0f});
            vertices.insert(vertices.end(), {x4, y4, z4, nx4, ny4, nz4, 0.0f, 1.0f});
        }
    }
    
    // 下半球
    for (int j = latitude_segments / 2; j < latitude_segments; ++j)
    {
        float phi1 = (float)j / latitude_segments * M_PI;
        float phi2 = (float)(j + 1) / latitude_segments * M_PI;
        
        for (int i = 0; i < longitude_segments; ++i)
        {
            float theta1 = (float)i / longitude_segments * 2.0f * M_PI;
            float theta2 = (float)(i + 1) / longitude_segments * 2.0f * M_PI;
            
            float x1 = radius * sinf(phi1) * cosf(theta1);
            float y1 = radius * cosf(phi1) - half_height;
            float z1 = radius * sinf(phi1) * sinf(theta1);
            
            float x2 = radius * sinf(phi1) * cosf(theta2);
            float y2 = radius * cosf(phi1) - half_height;
            float z2 = radius * sinf(phi1) * sinf(theta2);
            
            float x3 = radius * sinf(phi2) * cosf(theta2);
            float y3 = radius * cosf(phi2) - half_height;
            float z3 = radius * sinf(phi2) * sinf(theta2);
            
            float nx1 = sinf(phi1) * cosf(theta1);
            float ny1 = cosf(phi1);
            float nz1 = sinf(phi1) * sinf(theta1);
            
            float nx2 = sinf(phi1) * cosf(theta2);
            float ny2 = cosf(phi1);
            float nz2 = sinf(phi1) * sinf(theta2);
            
            float nx3 = sinf(phi2) * cosf(theta2);
            float ny3 = cosf(phi2);
            float nz3 = sinf(phi2) * sinf(theta2);
            
            vertices.insert(vertices.end(), {x1, y1, z1, nx1, ny1, nz1, 0.0f, 0.0f});
            vertices.insert(vertices.end(), {x2, y2, z2, nx2, ny2, nz2, 1.0f, 0.0f});
            vertices.insert(vertices.end(), {x3, y3, z3, nx3, ny3, nz3, 1.0f, 1.0f});
            
            float x4 = radius * sinf(phi2) * cosf(theta1);
            float y4 = radius * cosf(phi2) - half_height;
            float z4 = radius * sinf(phi2) * sinf(theta1);
            
            float nx4 = sinf(phi2) * cosf(theta1);
            float ny4 = cosf(phi2);
            float nz4 = sinf(phi2) * sinf(theta1);
            
            vertices.insert(vertices.end(), {x1, y1, z1, nx1, ny1, nz1, 0.0f, 0.0f});
            vertices.insert(vertices.end(), {x3, y3, z3, nx3, ny3, nz3, 1.0f, 1.0f});
            vertices.insert(vertices.end(), {x4, y4, z4, nx4, ny4, nz4, 0.0f, 1.0f});
        }
    }
    
    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &m_capsule_vao);
    glGenBuffers(1, &m_capsule_vbo);
    
    glBindVertexArray(m_capsule_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_capsule_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    m_capsule_vertex_count = vertices.size() / 8;
    cout << "胶囊体几何体创建完成: " << m_capsule_vertex_count << " 个顶点" << endl;
}

void Renderer::CreateDebugCollisionGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies,
                                           const std::map<std::pair<int, int>, float>& terrain_data)
{
    // 创建调试碰撞几何体
    UpdateDebugCollisionGeometry(active_bodies, terrain_data);
}

void Renderer::CreateCharacterDebugGeometry()
{
    cout << "创建角色碰撞体调试几何体..." << endl;
    
    std::vector<float> vertices;
    
    // 创建胶囊体的线框
    float capsule_radius = 0.3f;
    float capsule_half_height = 0.4f;
    int segments = 16;
    
    // 胶囊体圆柱部分
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (float)i / segments * 2.0f * M_PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * M_PI;
        
        float x1 = capsule_radius * cosf(angle1);
        float z1 = capsule_radius * sinf(angle1);
        float x2 = capsule_radius * cosf(angle2);
        float z2 = capsule_radius * sinf(angle2);
        
        // 圆柱底部边
        vertices.insert(vertices.end(), {x1, -capsule_half_height, z1, x2, -capsule_half_height, z2});
        // 圆柱顶部边
        vertices.insert(vertices.end(), {x1, capsule_half_height, z1, x2, capsule_half_height, z2});
        // 圆柱垂直边
        vertices.insert(vertices.end(), {x1, -capsule_half_height, z1, x1, capsule_half_height, z1});
    }
    
    // 上半球（经线）
    for (int i = 0; i < segments; ++i)
    {
        float angle = (float)i / segments * 2.0f * M_PI;
        
        for (int j = 0; j < segments / 2; ++j)
        {
            float phi1 = (float)j / segments * M_PI;
            float phi2 = (float)(j + 1) / segments * M_PI;
            
            float x1 = capsule_radius * sinf(phi1) * cosf(angle);
            float y1 = capsule_radius * cosf(phi1) + capsule_half_height;
            float z1 = capsule_radius * sinf(phi1) * sinf(angle);
            
            float x2 = capsule_radius * sinf(phi2) * cosf(angle);
            float y2 = capsule_radius * cosf(phi2) + capsule_half_height;
            float z2 = capsule_radius * sinf(phi2) * sinf(angle);
            
            vertices.insert(vertices.end(), {x1, y1, z1, x2, y2, z2});
        }
    }
    
    // 下半球（经线）
    for (int i = 0; i < segments; ++i)
    {
        float angle = (float)i / segments * 2.0f * M_PI;
        
        for (int j = segments / 2; j < segments; ++j)
        {
            float phi1 = (float)j / segments * M_PI;
            float phi2 = (float)(j + 1) / segments * M_PI;
            
            float x1 = capsule_radius * sinf(phi1) * cosf(angle);
            float y1 = capsule_radius * cosf(phi1) - capsule_half_height;
            float z1 = capsule_radius * sinf(phi1) * sinf(angle);
            
            float x2 = capsule_radius * sinf(phi2) * cosf(angle);
            float y2 = capsule_radius * cosf(phi2) - capsule_half_height;
            float z2 = capsule_radius * sinf(phi2) * sinf(angle);
            
            vertices.insert(vertices.end(), {x1, y1, z1, x2, y2, z2});
        }
    }
    
    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &m_character_debug_vao);
    glGenBuffers(1, &m_character_debug_vbo);
    
    glBindVertexArray(m_character_debug_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_character_debug_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // 只设置位置属性（线框不需要法线和纹理坐标）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    m_character_debug_vertex_count = vertices.size() / 3; // 每个顶点3个float
    
    cout << "角色碰撞体调试几何体创建完成: " << m_character_debug_vertex_count << " 个顶点" << endl;
}

void Renderer::CreateGroundCollisionSolidGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies,
                                                  const std::map<std::pair<int, int>, float>& terrain_data)
{
    // 创建地面碰撞实心几何体
    UpdateGroundCollisionSolidGeometry(active_bodies, terrain_data);
}

void Renderer::UpdateDebugCollisionGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies,
                                           const std::map<std::pair<int, int>, float>& terrain_data)
{
    // 删除旧的几何体
    if (m_debug_vao != 0)
    {
        glDeleteVertexArrays(1, &m_debug_vao);
        glDeleteBuffers(1, &m_debug_vbo);
        m_debug_vao = 0;
        m_debug_vbo = 0;
        m_debug_vertex_count = 0;
    }
    
    // 重新创建几何体
    std::vector<float> vertices;
    
    const float half_tile_size = 0.5f;  // terrain_tile_size / 2
    const float half_height = 0.5f;
    
    for (const auto& body_pair : active_bodies)
    {
        int grid_x = body_pair.first.first;
        int grid_z = body_pair.first.second;
        
        auto terrain_it = terrain_data.find({grid_x, grid_z});
        if (terrain_it == terrain_data.end())
            continue;
        
        float height = terrain_it->second;
        
        // 计算世界位置
        float world_x = (grid_x + 0.5f - 25.0f) * 1.0f;  // grid_size / 2 = 25
        float world_z = (grid_z + 0.5f - 25.0f) * 1.0f;
        
        float physics_center_y = height - half_height;
        float x1 = world_x - half_tile_size;
        float x2 = world_x + half_tile_size;
        float y1 = physics_center_y - half_height;
        float y2 = physics_center_y + half_height;
        float z1 = world_z - half_tile_size;
        float z2 = world_z + half_tile_size;
        
        // 添加盒子的12条边
        // 底面
        vertices.insert(vertices.end(), {x1, y1, z1, x2, y1, z1});
        vertices.insert(vertices.end(), {x2, y1, z1, x2, y1, z2});
        vertices.insert(vertices.end(), {x2, y1, z2, x1, y1, z2});
        vertices.insert(vertices.end(), {x1, y1, z2, x1, y1, z1});
        
        // 顶面
        vertices.insert(vertices.end(), {x1, y2, z1, x2, y2, z1});
        vertices.insert(vertices.end(), {x2, y2, z1, x2, y2, z2});
        vertices.insert(vertices.end(), {x2, y2, z2, x1, y2, z2});
        vertices.insert(vertices.end(), {x1, y2, z2, x1, y2, z1});
        
        // 垂直边
        vertices.insert(vertices.end(), {x1, y1, z1, x1, y2, z1});
        vertices.insert(vertices.end(), {x2, y1, z1, x2, y2, z1});
        vertices.insert(vertices.end(), {x2, y1, z2, x2, y2, z2});
        vertices.insert(vertices.end(), {x1, y1, z2, x1, y2, z2});
    }
    
    if (vertices.empty())
        return;
    
    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &m_debug_vao);
    glGenBuffers(1, &m_debug_vbo);
    
    glBindVertexArray(m_debug_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_debug_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // 设置顶点属性（只有位置）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    m_debug_vertex_count = vertices.size() / 3;
}

void Renderer::UpdateGroundCollisionSolidGeometry(const std::map<std::pair<int, int>, BodyID>& active_bodies,
                                                  const std::map<std::pair<int, int>, float>& terrain_data)
{
    // 删除旧的几何体
    if (m_ground_collision_vao != 0)
    {
        glDeleteVertexArrays(1, &m_ground_collision_vao);
        glDeleteBuffers(1, &m_ground_collision_vbo);
        m_ground_collision_vao = 0;
        m_ground_collision_vbo = 0;
        m_ground_collision_vertex_count = 0;
    }
    
    // 重新创建几何体
    std::vector<float> vertices;
    
    const float half_tile_size = 0.5f;
    const float half_height = 0.5f;
    
    for (const auto& body_pair : active_bodies)
    {
        int grid_x = body_pair.first.first;
        int grid_z = body_pair.first.second;
        
        auto terrain_it = terrain_data.find({grid_x, grid_z});
        if (terrain_it == terrain_data.end())
            continue;
        
        float height = terrain_it->second;
        
        // 计算世界位置
        float world_x = (grid_x + 0.5f - 25.0f) * 1.0f;
        float world_z = (grid_z + 0.5f - 25.0f) * 1.0f;
        
        float physics_center_y = height - half_height;
        float x1 = world_x - half_tile_size;
        float x2 = world_x + half_tile_size;
        float y1 = physics_center_y - half_height;
        float y2 = physics_center_y + half_height;
        float z1 = world_z - half_tile_size;
        float z2 = world_z + half_tile_size;
        
        // 添加盒子的6个面（每个面2个三角形，每个三角形3个顶点）
        // 每个顶点包含：位置(3) + 法线(3) + 纹理坐标(2) = 8个float
        // 所有面都使用逆时针（CCW）顶点顺序，确保法线朝外

        // 顶面 (Y = y2, 法线向上) - 逆时针从上方看
        vertices.insert(vertices.end(), {x1, y2, z1, 0, 1, 0, 0, 0}); // 左前
        vertices.insert(vertices.end(), {x1, y2, z2, 0, 1, 0, 0, 1}); // 左后
        vertices.insert(vertices.end(), {x2, y2, z2, 0, 1, 0, 1, 1}); // 右后
        vertices.insert(vertices.end(), {x1, y2, z1, 0, 1, 0, 0, 0}); // 左前
        vertices.insert(vertices.end(), {x2, y2, z2, 0, 1, 0, 1, 1}); // 右后
        vertices.insert(vertices.end(), {x2, y2, z1, 0, 1, 0, 1, 0}); // 右前

        // 底面 (Y = y1, 法线向下) - 逆时针从下方看
        vertices.insert(vertices.end(), {x1, y1, z1, 0, -1, 0, 0, 0}); // 左前
        vertices.insert(vertices.end(), {x2, y1, z1, 0, -1, 0, 1, 0}); // 右前
        vertices.insert(vertices.end(), {x2, y1, z2, 0, -1, 0, 1, 1}); // 右后
        vertices.insert(vertices.end(), {x1, y1, z1, 0, -1, 0, 0, 0}); // 左前
        vertices.insert(vertices.end(), {x2, y1, z2, 0, -1, 0, 1, 1}); // 右后
        vertices.insert(vertices.end(), {x1, y1, z2, 0, -1, 0, 0, 1}); // 左后

        // 前面 (Z = z1, 法线向前-Z) - 逆时针从前方看
        vertices.insert(vertices.end(), {x1, y1, z1, 0, 0, -1, 0, 0}); // 左下
        vertices.insert(vertices.end(), {x2, y1, z1, 0, 0, -1, 1, 0}); // 右下
        vertices.insert(vertices.end(), {x2, y2, z1, 0, 0, -1, 1, 1}); // 右上
        vertices.insert(vertices.end(), {x1, y1, z1, 0, 0, -1, 0, 0}); // 左下
        vertices.insert(vertices.end(), {x2, y2, z1, 0, 0, -1, 1, 1}); // 右上
        vertices.insert(vertices.end(), {x1, y2, z1, 0, 0, -1, 0, 1}); // 左上

        // 后面 (Z = z2, 法线向后+Z) - 逆时针从后方看
        vertices.insert(vertices.end(), {x2, y1, z2, 0, 0, 1, 0, 0}); // 右下（从后面看是左边）
        vertices.insert(vertices.end(), {x1, y1, z2, 0, 0, 1, 1, 0}); // 左下（从后面看是右边）
        vertices.insert(vertices.end(), {x1, y2, z2, 0, 0, 1, 1, 1}); // 左上（从后面看是右边）
        vertices.insert(vertices.end(), {x2, y1, z2, 0, 0, 1, 0, 0}); // 右下（从后面看是左边）
        vertices.insert(vertices.end(), {x1, y2, z2, 0, 0, 1, 1, 1}); // 左上（从后面看是右边）
        vertices.insert(vertices.end(), {x2, y2, z2, 0, 0, 1, 0, 1}); // 右上（从后面看是左边）

        // 左面 (X = x1, 法线向左-X) - 逆时针从左侧看
        vertices.insert(vertices.end(), {x1, y1, z2, -1, 0, 0, 0, 0}); // 后下
        vertices.insert(vertices.end(), {x1, y1, z1, -1, 0, 0, 1, 0}); // 前下
        vertices.insert(vertices.end(), {x1, y2, z1, -1, 0, 0, 1, 1}); // 前上
        vertices.insert(vertices.end(), {x1, y1, z2, -1, 0, 0, 0, 0}); // 后下
        vertices.insert(vertices.end(), {x1, y2, z1, -1, 0, 0, 1, 1}); // 前上
        vertices.insert(vertices.end(), {x1, y2, z2, -1, 0, 0, 0, 1}); // 后上

        // 右面 (X = x2, 法线向右+X) - 逆时针从右侧看
        vertices.insert(vertices.end(), {x2, y1, z1, 1, 0, 0, 0, 0}); // 前下
        vertices.insert(vertices.end(), {x2, y1, z2, 1, 0, 0, 1, 0}); // 后下
        vertices.insert(vertices.end(), {x2, y2, z2, 1, 0, 0, 1, 1}); // 后上
        vertices.insert(vertices.end(), {x2, y1, z1, 1, 0, 0, 0, 0}); // 前下
        vertices.insert(vertices.end(), {x2, y2, z2, 1, 0, 0, 1, 1}); // 后上
        vertices.insert(vertices.end(), {x2, y2, z1, 1, 0, 0, 0, 1}); // 前上
    }
    
    if (vertices.empty())
        return;
    
    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &m_ground_collision_vao);
    glGenBuffers(1, &m_ground_collision_vbo);
    
    glBindVertexArray(m_ground_collision_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_ground_collision_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // 设置顶点属性
    // 位置属性 (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 法线属性 (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // 纹理坐标属性 (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    m_ground_collision_vertex_count = vertices.size() / 8;
}

void Renderer::SetupOpenGLState()
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void Renderer::RenderTerrain(const Camera& camera, const TerrainSystem& terrain_system)
{
    GLuint ground_vao = terrain_system.GetGroundVAO();
    int ground_vertex_count = terrain_system.GetGroundVertexCount();
    GLuint ground_texture = terrain_system.GetGroundTexture();
    
    // 如果没有几何体，跳过渲染
    if (ground_vao == 0 || ground_vertex_count == 0)
        return;
    
    GLuint shader_program = m_shader_manager->GetShaderProgram();
    
    GLint model_loc = glGetUniformLocation(shader_program, "model");
    GLint object_color_loc = glGetUniformLocation(shader_program, "objectColor");
    GLint use_texture_loc = glGetUniformLocation(shader_program, "useTexture");
    GLint use_alpha_loc = glGetUniformLocation(shader_program, "useAlpha");
    
    glm::mat4 ground_model = glm::mat4(1.0f);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(ground_model));
    glUniform3f(object_color_loc, 1.0f, 1.0f, 1.0f);
    glUniform1i(use_texture_loc, 1);
    glUniform1i(use_alpha_loc, 0);
    
    if (ground_texture != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ground_texture);
        glUniform1i(glGetUniformLocation(shader_program, "textureSampler"), 0);
    }
    
    glBindVertexArray(ground_vao);
    glDrawArrays(GL_TRIANGLES, 0, ground_vertex_count);
}

void Renderer::RenderCharacter(const Camera& camera, const CharacterController& character_controller)
{
    GLuint shader_program = m_shader_manager->GetShaderProgram();
    
    GLint model_loc = glGetUniformLocation(shader_program, "model");
    GLint object_color_loc = glGetUniformLocation(shader_program, "objectColor");
    GLint use_texture_loc = glGetUniformLocation(shader_program, "useTexture");
    GLint use_alpha_loc = glGetUniformLocation(shader_program, "useAlpha");
    
    RVec3 char_pos = character_controller.GetPosition();
    glm::mat4 capsule_model = glm::translate(glm::mat4(1.0f), 
                                            glm::vec3((float)char_pos.GetX(), 
                                                     (float)char_pos.GetY(), 
                                                     (float)char_pos.GetZ()));
    
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(capsule_model));
    glUniform3f(object_color_loc, 0.4f, 0.4f, 1.0f); // 蓝色
    glUniform1i(use_texture_loc, 0);
    glUniform1i(use_alpha_loc, 0);
    
    glBindVertexArray(m_capsule_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_capsule_vertex_count);
}

void Renderer::RenderDebugCollisionMesh(const Camera& camera)
{
    if (m_debug_vao == 0 || m_debug_vertex_count == 0)
        return;
        
    GLuint shader_program = m_shader_manager->GetShaderProgram();
    
    GLint model_loc = glGetUniformLocation(shader_program, "model");
    GLint object_color_loc = glGetUniformLocation(shader_program, "objectColor");
    GLint use_texture_loc = glGetUniformLocation(shader_program, "useTexture");
    GLint use_alpha_loc = glGetUniformLocation(shader_program, "useAlpha");
    
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform3f(object_color_loc, 0.0f, 1.0f, 0.0f); // 绿色
    glUniform1i(use_texture_loc, 0);
    glUniform1i(use_alpha_loc, 0);
    
    glBindVertexArray(m_debug_vao);
    glDrawArrays(GL_LINES, 0, m_debug_vertex_count);
}

void Renderer::RenderGroundCollisionSolid(const Camera& camera, const TerrainSystem& terrain_system)
{
    if (m_ground_collision_vao == 0 || m_ground_collision_vertex_count == 0)
        return;
        
    GLuint shader_program = m_shader_manager->GetShaderProgram();
    
    GLint model_loc = glGetUniformLocation(shader_program, "model");
    GLint object_color_loc = glGetUniformLocation(shader_program, "objectColor");
    GLint use_texture_loc = glGetUniformLocation(shader_program, "useTexture");
    GLint use_alpha_loc = glGetUniformLocation(shader_program, "useAlpha");
    
    // 保存当前渲染状态
    GLboolean polygon_offset_enabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    
    // 稍微偏移深度，确保蓝色盒子在地形之上显示
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform3f(object_color_loc, 1.0f, 1.0f, 1.0f); // 白色（使用纹理）
    glUniform1i(use_texture_loc, 1); // 启用纹理
    glUniform1i(use_alpha_loc, 0);
    
    // 绑定地形纹理
    GLuint ground_texture = terrain_system.GetGroundTexture();
    if (ground_texture != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ground_texture);
        glUniform1i(glGetUniformLocation(shader_program, "textureSampler"), 0);
    }
    
    glBindVertexArray(m_ground_collision_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_ground_collision_vertex_count);
    
    // 恢复之前的渲染状态
    if (polygon_offset_enabled)
        glEnable(GL_POLYGON_OFFSET_FILL);
    else
        glDisable(GL_POLYGON_OFFSET_FILL);
}

void Renderer::RenderCharacterDebug(const Camera& camera, const CharacterController& character_controller)
{
    if (m_character_debug_vao == 0 || m_character_debug_vertex_count == 0)
        return;
        
    GLuint shader_program = m_shader_manager->GetShaderProgram();
    
    GLint model_loc = glGetUniformLocation(shader_program, "model");
    GLint object_color_loc = glGetUniformLocation(shader_program, "objectColor");
    GLint use_texture_loc = glGetUniformLocation(shader_program, "useTexture");
    GLint use_alpha_loc = glGetUniformLocation(shader_program, "useAlpha");
    
    RVec3 char_pos = character_controller.GetPosition();
    glm::mat4 character_debug_model = glm::translate(glm::mat4(1.0f), 
                                                     glm::vec3((float)char_pos.GetX(), 
                                                              (float)char_pos.GetY(), 
                                                              (float)char_pos.GetZ()));
    
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(character_debug_model));
    glUniform3f(object_color_loc, 1.0f, 0.0f, 0.0f); // 红色
    glUniform1i(use_texture_loc, 0);
    glUniform1i(use_alpha_loc, 0);
    
    glBindVertexArray(m_character_debug_vao);
    glDrawArrays(GL_LINES, 0, m_character_debug_vertex_count);
}

void Renderer::CleanupOpenGLResources()
{
    if (m_capsule_vao != 0)
    {
        glDeleteVertexArrays(1, &m_capsule_vao);
        glDeleteBuffers(1, &m_capsule_vbo);
        m_capsule_vao = 0;
        m_capsule_vbo = 0;
    }
    
    if (m_debug_vao != 0)
    {
        glDeleteVertexArrays(1, &m_debug_vao);
        glDeleteBuffers(1, &m_debug_vbo);
        m_debug_vao = 0;
        m_debug_vbo = 0;
    }
    
    if (m_character_debug_vao != 0)
    {
        glDeleteVertexArrays(1, &m_character_debug_vao);
        glDeleteBuffers(1, &m_character_debug_vbo);
        m_character_debug_vao = 0;
        m_character_debug_vbo = 0;
    }
    
    if (m_ground_collision_vao != 0)
    {
        glDeleteVertexArrays(1, &m_ground_collision_vao);
        glDeleteBuffers(1, &m_ground_collision_vbo);
        m_ground_collision_vao = 0;
        m_ground_collision_vbo = 0;
    }
}

void Renderer::AddQuadUltraFast(std::vector<float>& vertices, float x1, float y1, float z1, 
                               float x2, float y2, float z2, float x3, float y3, float z3, 
                               float x4, float y4, float z4, float nx, float ny, float nz)
{
    // 添加两个三角形组成一个四边形
    // 三角形1: v1, v2, v3
    vertices.insert(vertices.end(), {x1, y1, z1, nx, ny, nz, 0.0f, 0.0f});
    vertices.insert(vertices.end(), {x2, y2, z2, nx, ny, nz, 1.0f, 0.0f});
    vertices.insert(vertices.end(), {x3, y3, z3, nx, ny, nz, 1.0f, 1.0f});
    
    // 三角形2: v1, v3, v4
    vertices.insert(vertices.end(), {x1, y1, z1, nx, ny, nz, 0.0f, 0.0f});
    vertices.insert(vertices.end(), {x3, y3, z3, nx, ny, nz, 1.0f, 1.0f});
    vertices.insert(vertices.end(), {x4, y4, z4, nx, ny, nz, 0.0f, 1.0f});
}

void Renderer::CreateCollisionCheckerboardTexture(GLuint& texture_id)
{
    // 创建一个简单的棋盘纹理
    const int size = 16;
    unsigned char data[size * size * 3];
    
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            bool is_white = ((i / 4) + (j / 4)) % 2 == 0;
            unsigned char color = is_white ? 255 : 128;
            int idx = (i * size + j) * 3;
            data[idx] = color;
            data[idx + 1] = color;
            data[idx + 2] = color;
        }
    }
    
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

