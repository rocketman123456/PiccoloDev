#include "terrain_system.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>
#include <climits>
#include <cmath>

using namespace std;

// PerlinNoise 实现
PerlinNoise::PerlinNoise(unsigned int seed)
    : m_seed(seed)
{
    generatePermutationTable();
}

void PerlinNoise::setSeed(unsigned int seed)
{
    m_seed = seed;
    generatePermutationTable();
}

void PerlinNoise::generatePermutationTable()
{
    // 清空并重新初始化排列表
    p.clear();
    p.reserve(512); // 256 + 256 复制
    
    // 首先创建0-255的序列
    std::vector<int> temp(256);
    for (int i = 0; i < 256; ++i)
    {
        temp[i] = i;
    }
    
    // 使用种子进行洗牌
    unsigned int state = m_seed;
    for (int i = 255; i > 0; --i)
    {
        // 简单的线性同余生成器
        state = state * 1103515245 + 12345;
        int j = state % (i + 1);
        std::swap(temp[i], temp[j]);
    }
    
    // 将洗牌后的序列添加到排列表
    p.insert(p.end(), temp.begin(), temp.end());
    // 复制一份以简化索引计算
    p.insert(p.end(), temp.begin(), temp.end());
}

unsigned int PerlinNoise::simpleHash(unsigned int x)
{
    // 简单的哈希函数，用于生成伪随机数
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;
    return x;
}

double PerlinNoise::fade(double t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::lerp(double t, double a, double b)
{
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y, double z)
{
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double PerlinNoise::noise(double x, double y, double z)
{
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    double u = fade(x);
    double v = fade(y);
    double w = fade(z);

    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    return lerp(
        w,
        lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)), lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
        lerp(
            v,
            lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
            lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))
        )
    );
}

// TerrainSystem 实现
TerrainSystem::TerrainSystem()
    : m_ground_vao(0)
    , m_ground_vbo(0)
    , m_ground_vertex_count(0)
    , m_ground_texture(0)
    , m_last_terrain_center(-999, -999)
    , m_noise_generator(0) // 使用固定种子确保可重现的地形
{
}

TerrainSystem::~TerrainSystem()
{
    Shutdown();
}

bool TerrainSystem::Initialize()
{
    GenerateTerrainHeights();
    CreateCheckerboardTexture();
    UpdateGroundGeometry();
    return true;
}

void TerrainSystem::Update(const std::pair<int, int>& character_grid_pos)
{
    // 计算当前地形中心
    auto current_terrain_center = character_grid_pos;

    // 检查是否需要更新地形
    if (m_last_terrain_center != std::pair<int, int>{-999, -999})
    {
        int dx = abs(current_terrain_center.first - m_last_terrain_center.first);
        int dz = abs(current_terrain_center.second - m_last_terrain_center.second);

        // 如果角色没有移动足够远，不需要更新
        if (dx < m_config.update_threshold && dz < m_config.update_threshold)
            return;
    }


    // 计算新的区域
    std::set<std::pair<int, int>> new_terrain_region;
    for (int dx = -m_config.terrain_radius; dx <= m_config.terrain_radius; ++dx)
    {
        for (int dz = -m_config.terrain_radius; dz <= m_config.terrain_radius; ++dz)
        {
            int grid_x = current_terrain_center.first + dx;
            int grid_z = current_terrain_center.second + dz;
            new_terrain_region.insert({grid_x, grid_z});
        }
    }

    // 移除过远的地形数据
    auto it = m_generated_terrain.begin();
    while (it != m_generated_terrain.end())
    {
        if (new_terrain_region.find(it->first) == new_terrain_region.end())
        {
            it = m_generated_terrain.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 生成新的地形数据
    int new_terrain_count = 0;
    for (const auto& grid_pos : new_terrain_region)
    {
        if (m_generated_terrain.find(grid_pos) == m_generated_terrain.end())
        {
            float height = GenerateTerrainHeight(grid_pos.first, grid_pos.second);
            m_generated_terrain[grid_pos] = height;
            new_terrain_count++;
        }
    }

    m_last_terrain_center = current_terrain_center;


    // 地形更新后，更新地面几何体
    UpdateGroundGeometry();
}

void TerrainSystem::Shutdown()
{
    CleanupOpenGLResources();
}

void TerrainSystem::GenerateTerrainHeights()
{
    // 为每个地形块生成一个高度
    int num_tiles = m_config.grid_size * m_config.grid_size;
    m_terrain_heights.resize(num_tiles);

    // 噪声参数
    float scale = 0.05f; // 控制噪声频率
    float height_range = m_config.max_height - m_config.min_height; // 高度变化范围
    float base_height = (m_config.min_height + m_config.max_height) * 0.5f; // 基础高度（中点）

    for (int z = 0; z < m_config.grid_size; ++z)
    {
        for (int x = 0; x < m_config.grid_size; ++x)
        {
            int index = z * m_config.grid_size + x;

            // 计算噪声值
            double noise_value = m_noise_generator.noise(x * scale, z * scale, 0.0);

            // 将噪声值转换为高度（范围在 min_height 到 max_height 之间）
            float height = base_height + noise_value * height_range * 0.5f;

            // 量化为配置的高度步长
            height = QuantizeHeight(height);

            m_terrain_heights[index] = height;
            
            // 同时填充生成的地形映射，用于几何体创建
            m_generated_terrain[{x, z}] = height;
        }
    }
    
}

float TerrainSystem::GenerateTerrainHeight(int grid_x, int grid_z)
{
    // 使用简单的噪声函数生成地形高度
    float noise1 = sin(grid_x * 0.1f) * cos(grid_z * 0.1f);
    float noise2 = sin(grid_x * 0.05f) * cos(grid_z * 0.05f) * 0.5f;
    float noise3 = sin(grid_x * 0.02f) * cos(grid_z * 0.02f) * 0.25f;

    float height_range = m_config.max_height - m_config.min_height;
    float base_height = (m_config.min_height + m_config.max_height) * 0.5f;
    float height = (noise1 + noise2 + noise3) * height_range * 0.5f + base_height;

    // 量化为配置的高度步长
    return QuantizeHeight(height);
}

float TerrainSystem::QuantizeHeight(float height)
{
    // 确保高度在配置范围内
    height = std::max(m_config.min_height, std::min(m_config.max_height, height));
    
    // 量化为配置的高度步长
    float quantized_height = m_config.min_height + 
        std::round((height - m_config.min_height) / m_config.height_step) * m_config.height_step;
    
    // 确保量化后的高度仍在范围内
    return std::max(m_config.min_height, std::min(m_config.max_height, quantized_height));
}

float TerrainSystem::GetTerrainHeightAtWorldPos(float world_x, float world_z) const
{
    // 将世界坐标转换为网格坐标
    int grid_x = static_cast<int>(std::round(world_x / m_config.tile_size + m_config.grid_size / 2.0f));
    int grid_z = static_cast<int>(std::round(world_z / m_config.tile_size + m_config.grid_size / 2.0f));
    
    // 优先使用动态生成的地形数据
    auto terrain_it = m_generated_terrain.find({grid_x, grid_z});
    if (terrain_it != m_generated_terrain.end())
    {
        return terrain_it->second;
    }
    
    // 如果动态数据中没有，尝试使用固定地形数据
    if (grid_x >= 0 && grid_x < m_config.grid_size && grid_z >= 0 && grid_z < m_config.grid_size)
    {
        int tile_idx = grid_z * m_config.grid_size + grid_x;
        if (tile_idx >= 0 && tile_idx < static_cast<int>(m_terrain_heights.size()))
        {
            return m_terrain_heights[tile_idx];
        }
    }
    
    // 如果都没有，使用默认高度
    return m_config.min_height;
}

void TerrainSystem::UpdateGroundGeometry()
{
    // 调用不带视锥剔除的版本
    UpdateGroundGeometry(glm::mat4(1.0f));
}

void TerrainSystem::UpdateGroundGeometry(const glm::mat4& view_proj_matrix)
{
    // 删除旧的几何体（但保留纹理）
    if (m_ground_vao != 0)
    {
        glDeleteVertexArrays(1, &m_ground_vao);
        m_ground_vao = 0;
    }
    if (m_ground_vbo != 0)
    {
        glDeleteBuffers(1, &m_ground_vbo);
        m_ground_vbo = 0;
    }
    m_ground_vertex_count = 0;

    if (m_generated_terrain.empty()) return;

    // 找到地形数据的边界
    int min_x = INT_MAX, max_x = INT_MIN;
    int min_z = INT_MAX, max_z = INT_MIN;

    for (const auto& terrain_pair : m_generated_terrain)
    {
        int x = terrain_pair.first.first;
        int z = terrain_pair.first.second;
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_z = std::min(min_z, z);
        max_z = std::max(max_z, z);
    }


    // 估算顶点数量
    int estimated_vertices = 0;
    for (int z = min_z; z <= max_z; ++z)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            auto terrain_it = m_generated_terrain.find({x, z});
            if (terrain_it == m_generated_terrain.end())
                continue;

            float height = terrain_it->second;

            // 每个块至少有顶面（6个顶点）
            estimated_vertices += 6;

            // 检查侧面是否需要渲染
            auto left_it = m_generated_terrain.find({x - 1, z});
            if (left_it == m_generated_terrain.end() || left_it->second < height)
                estimated_vertices += 6; // 左面

            auto right_it = m_generated_terrain.find({x + 1, z});
            if (right_it == m_generated_terrain.end() || right_it->second < height)
                estimated_vertices += 6; // 右面

            auto front_it = m_generated_terrain.find({x, z - 1});
            if (front_it == m_generated_terrain.end() || front_it->second < height)
                estimated_vertices += 6; // 前面

            auto back_it = m_generated_terrain.find({x, z + 1});
            if (back_it == m_generated_terrain.end() || back_it->second < height)
                estimated_vertices += 6; // 后面
        }
    }

    // 预分配内存
    std::vector<float> vertices;
    vertices.reserve(estimated_vertices * 8); // 每个顶点8个float

    cout << "预分配内存: " << estimated_vertices << " 个顶点" << endl;

    int processed_tiles = 0;
    int total_tiles = (max_x - min_x + 1) * (max_z - min_z + 1);

    // 生成顶点数据
    for (int z = min_z; z <= max_z; ++z)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            auto terrain_it = m_generated_terrain.find({x, z});
            if (terrain_it == m_generated_terrain.end())
                continue;

            // 视锥剔除检查
            if (m_config.enable_frustum_culling && !IsTerrainTileVisible(x, z, view_proj_matrix))
                continue;

            float height = terrain_it->second;

            // 计算世界坐标
            float world_x1 = (x - m_config.grid_size / 2.0f) * m_config.tile_size;
            float world_x2 = (x + 1 - m_config.grid_size / 2.0f) * m_config.tile_size;
            float world_z1 = (z - m_config.grid_size / 2.0f) * m_config.tile_size;
            float world_z2 = (z + 1 - m_config.grid_size / 2.0f) * m_config.tile_size;

            float height_top = height;
            float height_bottom = m_config.base_height;

            // 顶面（总是渲染）
            AddQuadUltraFast(vertices,
                world_x1, height_top, world_z1,
                world_x2, height_top, world_z1,
                world_x2, height_top, world_z2,
                world_x1, height_top, world_z2,
                0.0f, 1.0f, 0.0f
            );

            // 底面（总是渲染）
            AddQuadUltraFast(vertices,
                world_x1, height_bottom, world_z1,
                world_x1, height_bottom, world_z2,
                world_x2, height_bottom, world_z2,
                world_x2, height_bottom, world_z1,
                0.0f, -1.0f, 0.0f
            );

            // 左面 (-X) - 只在高度变化时渲染
            auto left_it = m_generated_terrain.find({x - 1, z});
            float left_height = (left_it != m_generated_terrain.end()) ? left_it->second : m_config.base_height;
            if (left_height != height_top)
            {
                AddQuadUltraFast(vertices,
                    world_x1, height_bottom, world_z1,
                    world_x1, height_bottom, world_z2,
                    world_x1, height_top, world_z2,
                    world_x1, height_top, world_z1,
                    -1.0f, 0.0f, 0.0f
                );
            }

            // 右面 (+X) - 只在高度变化时渲染
            auto right_it = m_generated_terrain.find({x + 1, z});
            float right_height = (right_it != m_generated_terrain.end()) ? right_it->second : m_config.base_height;
            if (right_height != height_top)
            {
                AddQuadUltraFast(vertices,
                    world_x2, height_bottom, world_z1,
                    world_x2, height_bottom, world_z2,
                    world_x2, height_top, world_z2,
                    world_x2, height_top, world_z1,
                    1.0f, 0.0f, 0.0f
                );
            }

            // 前面 (-Z) - 只在高度变化时渲染
            auto front_it = m_generated_terrain.find({x, z - 1});
            float front_height = (front_it != m_generated_terrain.end()) ? front_it->second : m_config.base_height;
            if (front_height != height_top)
            {
                AddQuadUltraFast(vertices,
                    world_x1, height_bottom, world_z1,
                    world_x2, height_bottom, world_z1,
                    world_x2, height_top, world_z1,
                    world_x1, height_top, world_z1,
                    0.0f, 0.0f, -1.0f
                );
            }

            // 后面 (+Z) - 只在高度变化时渲染
            auto back_it = m_generated_terrain.find({x, z + 1});
            float back_height = (back_it != m_generated_terrain.end()) ? back_it->second : m_config.base_height;
            if (back_height != height_top)
            {
                AddQuadUltraFast(vertices,
                    world_x1, height_bottom, world_z2,
                    world_x2, height_bottom, world_z2,
                    world_x2, height_top, world_z2,
                    world_x1, height_top, world_z2,
                    0.0f, 0.0f, 1.0f
                );
            }

            processed_tiles++;
        }
    }

    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &m_ground_vao);
    glGenBuffers(1, &m_ground_vbo);

    glBindVertexArray(m_ground_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_ground_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 设置顶点属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m_ground_vertex_count = static_cast<int>(vertices.size() / 8); // 8 floats per vertex

    cout << "动态地面几何体创建完成: " << m_ground_vertex_count << " 个顶点" << endl;
    cout << "内存使用: " << (vertices.size() * sizeof(float) / 1024.0f / 1024.0f) << " MB" << endl;
}

void TerrainSystem::CreateGroundGeometry()
{
    // 这个方法现在被 UpdateGroundGeometry 替代
    UpdateGroundGeometry();
}

bool TerrainSystem::IsTerrainTileVisible(int x, int z, const glm::mat4& view_proj_matrix) const
{
    // 计算地形块的世界坐标
    float world_x1 = (x - m_config.grid_size / 2.0f) * m_config.tile_size;
    float world_x2 = (x + 1 - m_config.grid_size / 2.0f) * m_config.tile_size;
    float world_z1 = (z - m_config.grid_size / 2.0f) * m_config.tile_size;
    float world_z2 = (z + 1 - m_config.grid_size / 2.0f) * m_config.tile_size;

    // 获取地形块高度 - 优先使用动态生成的地形数据
    float height_top;
    auto terrain_it = m_generated_terrain.find({x, z});
    if (terrain_it != m_generated_terrain.end())
    {
        height_top = terrain_it->second;
    }
    else
    {
        // 如果动态数据中没有，尝试使用固定地形数据
        int tile_idx = z * m_config.grid_size + x;
        if (tile_idx >= 0 && tile_idx < static_cast<int>(m_terrain_heights.size()))
        {
            height_top = m_terrain_heights[tile_idx];
        }
        else
        {
            height_top = m_config.min_height; // 使用默认高度
        }
    }
    float height_bottom = m_config.base_height;

    // 地形块的8个角点
    glm::vec4 corners[8] = {
        glm::vec4(world_x1, height_bottom, world_z1, 1.0f), // 左下前
        glm::vec4(world_x2, height_bottom, world_z1, 1.0f), // 右下前
        glm::vec4(world_x2, height_bottom, world_z2, 1.0f), // 右下后
        glm::vec4(world_x1, height_bottom, world_z2, 1.0f), // 左下后
        glm::vec4(world_x1, height_top, world_z1, 1.0f),    // 左上前
        glm::vec4(world_x2, height_top, world_z1, 1.0f),    // 右上前
        glm::vec4(world_x2, height_top, world_z2, 1.0f),    // 右上后
        glm::vec4(world_x1, height_top, world_z2, 1.0f)     // 左上后
    };

    // 将角点变换到裁剪空间
    bool any_inside = false;
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 clip_pos = view_proj_matrix * corners[i];

        // 检查是否在视锥内（-w < x,y,z < w）
        // 使用配置的容错范围以避免边界问题
        float tolerance = m_config.frustum_culling_tolerance;
        if (clip_pos.x >= -clip_pos.w - tolerance && clip_pos.x <= clip_pos.w + tolerance && 
            clip_pos.y >= -clip_pos.w - tolerance && clip_pos.y <= clip_pos.w + tolerance && 
            clip_pos.z >= -clip_pos.w - tolerance && clip_pos.z <= clip_pos.w + tolerance)
        {
            any_inside = true;
            break;
        }
    }

    return any_inside;
}

void TerrainSystem::CreateCheckerboardTexture()
{
    const int size = 128;  // 增加纹理分辨率
    unsigned char data[size * size * 3];

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            int idx = (y * size + x) * 3;
            // 创建更明显的棋盘格图案
            int checker_size = 16;  // 每个棋盘格的大小
            bool is_white = ((x / checker_size) + (y / checker_size)) % 2 == 0;
            
            if (is_white)
            {
                // 白色格子 - 更亮的颜色
                data[idx + 0] = 255;  // R
                data[idx + 1] = 255;  // G
                data[idx + 2] = 255;  // B
            }
            else
            {
                // 黑色格子 - 更暗的颜色
                data[idx + 0] = 64;   // R
                data[idx + 1] = 64;   // G
                data[idx + 2] = 64;   // B
            }
        }
    }

    glGenTextures(1, &m_ground_texture);
    glBindTexture(GL_TEXTURE_2D, m_ground_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    cout << "棋盘纹理创建完成: " << size << "x" << size << " 像素" << endl;
}

void TerrainSystem::AddQuadUltraFast(std::vector<float>& vertices, float x1, float y1, float z1, float x2, float y2, float z2, 
                                     float x3, float y3, float z3, float x4, float y4, float z4, float nx, float ny, float nz)
{
    // 计算基于世界坐标的UV坐标，使棋盘纹理重复
    float uv_scale = 0.5f;  // 控制纹理重复频率，值越小重复越多
    
    // 根据法线方向选择正确的UV坐标
    float u1, v1, u2, v2, u3, v3, u4, v4;
    
    if (fabs(ny) > 0.9f)
    {
        // 顶面或底面（法线主要沿Y轴）- 使用 X-Z 平面坐标
        u1 = x1 * uv_scale; v1 = z1 * uv_scale;
        u2 = x2 * uv_scale; v2 = z2 * uv_scale;
        u3 = x3 * uv_scale; v3 = z3 * uv_scale;
        u4 = x4 * uv_scale; v4 = z4 * uv_scale;
    }
    else if (fabs(nx) > 0.9f)
    {
        // 左面或右面（法线主要沿X轴）- 使用 Z-Y 平面坐标
        u1 = z1 * uv_scale; v1 = y1 * uv_scale;
        u2 = z2 * uv_scale; v2 = y2 * uv_scale;
        u3 = z3 * uv_scale; v3 = y3 * uv_scale;
        u4 = z4 * uv_scale; v4 = y4 * uv_scale;
    }
    else // fabs(nz) > 0.9f
    {
        // 前面或后面（法线主要沿Z轴）- 使用 X-Y 平面坐标
        u1 = x1 * uv_scale; v1 = y1 * uv_scale;
        u2 = x2 * uv_scale; v2 = y2 * uv_scale;
        u3 = x3 * uv_scale; v3 = y3 * uv_scale;
        u4 = x4 * uv_scale; v4 = y4 * uv_scale;
    }
    
    // 直接添加48个float（6个顶点 * 8个float/顶点）
    vertices.insert(
        vertices.end(),
        {
            x1, y1, z1, nx, ny, nz, u1, v1, // 顶点1
            x2, y2, z2, nx, ny, nz, u2, v2, // 顶点2
            x3, y3, z3, nx, ny, nz, u3, v3, // 顶点3
            x1, y1, z1, nx, ny, nz, u1, v1, // 顶点4
            x3, y3, z3, nx, ny, nz, u3, v3, // 顶点5
            x4, y4, z4, nx, ny, nz, u4, v4  // 顶点6
        }
    );
}

void TerrainSystem::CleanupOpenGLResources()
{
    if (m_ground_vao != 0)
    {
        glDeleteVertexArrays(1, &m_ground_vao);
        m_ground_vao = 0;
    }
    if (m_ground_vbo != 0)
    {
        glDeleteBuffers(1, &m_ground_vbo);
        m_ground_vbo = 0;
    }
    if (m_ground_texture != 0)
    {
        glDeleteTextures(1, &m_ground_texture);
        m_ground_texture = 0;
    }
    m_ground_vertex_count = 0;
}
