#pragma once

#include "jolt_character_test_common.h"

#include <vector>

// Perlin噪声生成器
class PerlinNoise
{
private:
    std::vector<int> p;
    unsigned int m_seed;

public:
    PerlinNoise(unsigned int seed = 0);
    double noise(double x, double y, double z);
    void setSeed(unsigned int seed);

private:
    double fade(double t);
    double lerp(double t, double a, double b);
    double grad(int hash, double x, double y, double z);
    void generatePermutationTable();
    unsigned int simpleHash(unsigned int x);
};

// 地形系统
class TerrainSystem
{
public:
    TerrainSystem();
    ~TerrainSystem();

    bool Initialize();
    void Update(const std::pair<int, int>& character_grid_pos);
    void Shutdown();

    // 地形生成
    void GenerateTerrainHeights();
    float GenerateTerrainHeight(int grid_x, int grid_z);
    float QuantizeHeight(float height);
    
    // 地形高度查询
    float GetTerrainHeightAtWorldPos(float world_x, float world_z) const;
    
    // 地形数据访问
    const std::map<std::pair<int, int>, float>& GetGeneratedTerrain() const { return m_generated_terrain; }
    const std::vector<float>& GetTerrainHeights() const { return m_terrain_heights; }
    
    // 配置管理
    void SetConfig(const TerrainConfig& config) { m_config = config; }
    const TerrainConfig& GetConfig() const { return m_config; }
    
    // 几何体管理
    void UpdateGroundGeometry();
    void UpdateGroundGeometry(const glm::mat4& view_proj_matrix);
    void CreateGroundGeometry();
    
    // OpenGL资源
    GLuint GetGroundVAO() const { return m_ground_vao; }
    GLuint GetGroundVBO() const { return m_ground_vbo; }
    int GetGroundVertexCount() const { return m_ground_vertex_count; }
    GLuint GetGroundTexture() const { return m_ground_texture; }

    // 视锥剔除
    bool IsTerrainTileVisible(int x, int z, const glm::mat4& view_proj_matrix) const;

private:
    // 地形数据
    std::map<std::pair<int, int>, float> m_generated_terrain;
    std::vector<float> m_terrain_heights;
    std::pair<int, int> m_last_terrain_center;
    
    // OpenGL资源
    GLuint m_ground_vao;
    GLuint m_ground_vbo;
    int m_ground_vertex_count;
    GLuint m_ground_texture;
    
    // 配置
    TerrainConfig m_config;
    
    // 噪声生成器
    PerlinNoise m_noise_generator;

    // 内部方法
    void CreateCheckerboardTexture();
    void AddQuadUltraFast(std::vector<float>& vertices, float x1, float y1, float z1, float x2, float y2, float z2, 
                         float x3, float y3, float z3, float x4, float y4, float z4, float nx, float ny, float nz);
    void CleanupOpenGLResources();
};
