#include "terrain_system.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>
#include <climits>
#include <cmath>

using namespace std;

// PerlinNoise 实现
PerlinNoise::PerlinNoise()
{
    // 初始化排列表
    p = {151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,
         190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,
         125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,
         41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130,
         116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207,
         206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,
         172, 9,   129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144,
         12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,
         127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180};

    // 复制排列表
    p.insert(p.end(), p.begin(), p.end());
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
{
}

TerrainSystem::~TerrainSystem()
{
    Shutdown();
}

bool TerrainSystem::Initialize()
{
    cout << "初始化地形系统..." << endl;
    
    // 生成地形高度数据
    GenerateTerrainHeights();
    
    // 创建棋盘格纹理
    CreateCheckerboardTexture();
    
    // 创建初始地面几何体
    UpdateGroundGeometry();
    
    cout << "地形系统初始化完成" << endl;
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

    cout << "更新地形，角色位置: (" << character_grid_pos.first << ", " << character_grid_pos.second << ")" << endl;

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

    cout << "地形更新完成，新增地形块: " << new_terrain_count << "，总地形块: " << m_generated_terrain.size() << endl;

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
    float amplitude = 5.0f; // 控制高度变化幅度
    float base_height = 2.0f; // 基础高度

    for (int z = 0; z < m_config.grid_size; ++z)
    {
        for (int x = 0; x < m_config.grid_size; ++x)
        {
            int index = z * m_config.grid_size + x;

            // 计算噪声值
            double noise_value = m_noise_generator.noise(x * scale, z * scale, 0.0);

            // 将噪声值转换为高度（范围约 1.0 到 3.0）
            float height = base_height + noise_value * amplitude;

            // 量化为整数高度：1.0, 2.0, 3.0
            if (height < 1.5f)
                height = 1.0f;
            else if (height < 2.5f)
                height = 2.0f;
            else if (height < 3.5f)
                height = 3.0f;
            else if (height < 4.5f)
                height = 4.0f;
            else
                height = 5.0f;

            m_terrain_heights[index] = height;
            
            // 同时填充生成的地形映射，用于几何体创建
            m_generated_terrain[{x, z}] = height;
        }
    }
    
    cout << "生成了 " << m_generated_terrain.size() << " 个地形块" << endl;
}

float TerrainSystem::GenerateTerrainHeight(int grid_x, int grid_z)
{
    // 使用简单的噪声函数生成地形高度
    float noise1 = sin(grid_x * 0.1f) * cos(grid_z * 0.1f);
    float noise2 = sin(grid_x * 0.05f) * cos(grid_z * 0.05f) * 0.5f;
    float noise3 = sin(grid_x * 0.02f) * cos(grid_z * 0.02f) * 0.25f;

    float height = (noise1 + noise2 + noise3) * 2.0f + 3.0f; // 基础高度3.0，变化范围约±3.0

    // 量化为整数高度
    return floor(height);
}

void TerrainSystem::UpdateGroundGeometry()
{
    cout << "更新地面几何体（基于动态地形）..." << endl;

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

    // 计算需要渲染的地形区域（基于生成的地形数据）
    if (m_generated_terrain.empty())
    {
        cout << "没有生成的地形数据，跳过地面几何体创建" << endl;
        return;
    }

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

    cout << "地形数据范围: X[" << min_x << ", " << max_x << "], Z[" << min_z << ", " << max_z << "]" << endl;

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

            float height = terrain_it->second;

            // 计算世界坐标
            float world_x1 = (x - m_config.grid_size / 2.0f) * m_config.tile_size;
            float world_x2 = (x + 1 - m_config.grid_size / 2.0f) * m_config.tile_size;
            float world_z1 = (z - m_config.grid_size / 2.0f) * m_config.tile_size;
            float world_z2 = (z + 1 - m_config.grid_size / 2.0f) * m_config.tile_size;

            float height_top = height;
            float height_bottom = height - 1.0f;

            // 顶面（总是渲染）
            AddQuadUltraFast(vertices,
                world_x1, height_top, world_z1,
                world_x2, height_top, world_z1,
                world_x2, height_top, world_z2,
                world_x1, height_top, world_z2,
                0.0f, 1.0f, 0.0f
            );

            // 左面 (-X) - 只在高度变化时渲染
            auto left_it = m_generated_terrain.find({x - 1, z});
            if (left_it == m_generated_terrain.end() || left_it->second < height_top)
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
            if (right_it == m_generated_terrain.end() || right_it->second < height_top)
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
            if (front_it == m_generated_terrain.end() || front_it->second < height_top)
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
            if (back_it == m_generated_terrain.end() || back_it->second < height_top)
            {
                AddQuadUltraFast(vertices,
                    world_x1, height_bottom, world_z2,
                    world_x2, height_bottom, world_z2,
                    world_x2, height_top, world_z2,
                    world_x1, height_top, world_z2,
                    0.0f, 0.0f, 1.0f
                );
            }

            // 进度显示
            processed_tiles++;
            if (processed_tiles % (total_tiles / 10) == 0)
            {
                int progress = (processed_tiles * 100) / total_tiles;
                cout << "进度: " << progress << "% (" << processed_tiles << "/" << total_tiles << ")" << endl;
            }
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

    // 获取地形块高度
    int tile_idx = z * m_config.grid_size + x;
    float height_top = m_terrain_heights[tile_idx];
    float height_bottom = height_top - 1.0f;

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
    bool all_outside = true;
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 clip_pos = view_proj_matrix * corners[i];

        // 检查是否在视锥内（-w < x,y,z < w）
        if (clip_pos.x >= -clip_pos.w && clip_pos.x <= clip_pos.w && 
            clip_pos.y >= -clip_pos.w && clip_pos.y <= clip_pos.w && 
            clip_pos.z >= -clip_pos.w && clip_pos.z <= clip_pos.w)
        {
            all_outside = false;
            break;
        }
    }

    return !all_outside;
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
