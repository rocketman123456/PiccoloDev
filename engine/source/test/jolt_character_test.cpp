// Jolt Physics Character Controller Example
// 使用 GLFW 和 OpenGL 3.3 Core Profile 渲染一个在平坦地面上移动的胶囊体角色

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/Character.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <vector>
#include <random>

using namespace JPH;
using namespace std;

// 层定义
namespace Layers
{
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING     = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

// 对象层过滤器
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// 宽相层过滤器
namespace BPLayers
{
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint            NUM_LAYERS(2);
}; // namespace BPLayers

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        mObjectToBroadPhase[Layers::NON_MOVING] = BPLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING]     = BPLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override { return BPLayers::NUM_LAYERS; }

    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
    {
        switch ((BroadPhaseLayer::Type)inLayer)
        {
            case (BroadPhaseLayer::Type)BPLayers::NON_MOVING:
                return "NON_MOVING";
            case (BroadPhaseLayer::Type)BPLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif

private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// 对象与宽相层过滤器
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
            case Layers::NON_MOVING:
                return inLayer2 == BPLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// 联系监听器
class MyContactListener : public ContactListener
{
public:
    virtual ValidateResult
    OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override
    {
        return ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override {}

    virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override {}

    virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override {}
};

// 身体激活监听器
class MyBodyActivationListener : public BodyActivationListener
{
public:
    virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override {}

    virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override {}
};

// 全局变量
GLFWwindow*          g_window         = nullptr;
PhysicsSystem*       g_physics_system = nullptr;
Character*           g_character      = nullptr;
TempAllocatorImpl*   g_temp_allocator = nullptr;
JobSystemThreadPool* g_job_system     = nullptr;
GLuint               g_ground_texture = 0;

// 着色器和渲染资源
GLuint g_shader_program       = 0;
GLuint g_ground_vao           = 0;
GLuint g_ground_vbo           = 0;
GLuint g_capsule_vao          = 0;
GLuint g_capsule_vbo          = 0;
int    g_capsule_vertex_count = 0;

// 地形参数
const int           g_terrain_grid_size = 20;   // 20x20 地形块
const float         g_terrain_tile_size = 1.0f;  // 每块 1m x 1m
std::vector<float>  g_terrain_heights;            // 存储每个网格点的高度

// 摄像机参数
glm::vec3 g_camera_pos(0, 5, 10);
glm::vec3 g_camera_target(0, 1, 0);
float     g_camera_yaw      = -90.0f; // 偏航角（左右旋转）
float     g_camera_pitch    = -20.0f; // 俯仰角（上下旋转）
float     g_camera_distance = 20.0f;  // 摄像机距离角色的距离（适应100x100地形）
bool      g_mouse_captured  = false;  // 鼠标是否被捕获
double    g_last_mouse_x    = 400.0;
double    g_last_mouse_y    = 300.0;
bool      g_first_mouse     = true;

// 输入状态
bool g_key_w     = false;
bool g_key_s     = false;
bool g_key_a     = false;
bool g_key_d     = false;
bool g_key_space = false;

// 鼠标移动回调
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!g_mouse_captured)
        return;

    if (g_first_mouse)
    {
        g_last_mouse_x = xpos;
        g_last_mouse_y = ypos;
        g_first_mouse  = false;
        return;
    }

    float xoffset  = xpos - g_last_mouse_x;
    float yoffset  = g_last_mouse_y - ypos; // 反转 Y 轴
    g_last_mouse_x = xpos;
    g_last_mouse_y = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    g_camera_yaw += xoffset;
    g_camera_pitch += yoffset;

    // 限制俯仰角范围
    if (g_camera_pitch > 89.0f)
        g_camera_pitch = 89.0f;
    if (g_camera_pitch < -89.0f)
        g_camera_pitch = -89.0f;
}

// 鼠标滚轮回调
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_camera_distance -= (float)yoffset;
    if (g_camera_distance < 2.0f)
        g_camera_distance = 2.0f;
    if (g_camera_distance > 50.0f)
        g_camera_distance = 50.0f;
}

// 键盘回调
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

    if (key == GLFW_KEY_W)
        g_key_w = pressed;
    if (key == GLFW_KEY_S)
        g_key_s = pressed;
    if (key == GLFW_KEY_A)
        g_key_a = pressed;
    if (key == GLFW_KEY_D)
        g_key_d = pressed;
    if (key == GLFW_KEY_SPACE)
        g_key_space = pressed;

    // P 键切换鼠标捕获
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_mouse_captured = !g_mouse_captured;
        if (g_mouse_captured)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            g_first_mouse = true;
            cout << "鼠标已捕获 - 可以旋转摄像机" << endl;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cout << "鼠标已释放" << endl;
        }
    }
}

// 编译着色器
GLuint compile_shader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "着色器编译失败: " << info_log << std::endl;
    }

    return shader;
}

// 创建着色器程序
GLuint create_shader_program()
{
    const char* vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        
        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const char* fragment_shader_source = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 objectColor;
        uniform bool useTexture;
        uniform sampler2D textureSampler;
        uniform bool useAlpha;
        uniform float alpha;
        
        void main()
        {
            vec3 color = objectColor;
            if (useTexture)
            {
                color = texture(textureSampler, TexCoord).rgb;
            }
            
            if (useAlpha)
            {
                FragColor = vec4(color, alpha);
                return;
            }
            
            // 环境光
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * color;
            
            // 漫反射
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * color;
            
            // 镜面反射
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0);
            
            vec3 result = ambient + diffuse + specular;
            FragColor = vec4(result, 1.0);
        }
    )";

    GLuint vertex_shader   = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        std::cerr << "着色器程序链接失败: " << info_log << std::endl;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

// 创建棋盘格纹理
GLuint create_checkerboard_texture()
{
    const int     size = 64;
    unsigned char data[size * size * 3];

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            int idx = (y * size + x) * 3;
            // 创建棋盘格图案
            bool          is_white = ((x / 8) + (y / 8)) % 2 == 0;
            unsigned char color    = is_white ? 200 : 150;
            data[idx + 0]          = color;
            data[idx + 1]          = color;
            data[idx + 2]          = color;
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return texture;
}

// 简单的 Perlin 噪声实现
class PerlinNoise
{
private:
    std::vector<int> p;

public:
    PerlinNoise()
    {
        // 初始化排列表
        p = {
            151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
            140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
            247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
            57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
            74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
            60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
            65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
            200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
            52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
            207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
            119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
            129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
            218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
            81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
            184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
            222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
        };
        
        // 复制排列表
        p.insert(p.end(), p.begin(), p.end());
    }

    double fade(double t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    double lerp(double t, double a, double b)
    {
        return a + t * (b - a);
    }

    double grad(int hash, double x, double y, double z)
    {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    double noise(double x, double y, double z)
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

        return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                       grad(p[BA], x - 1, y, z)),
                               lerp(u, grad(p[AB], x, y - 1, z),
                                       grad(p[BB], x - 1, y - 1, z))),
                       lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                       grad(p[BA + 1], x - 1, y, z - 1)),
                               lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                       grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }
};

// 生成地形高度数据（使用 Perlin 噪声，量化为整数高度）
void generate_terrain_heights()
{
    PerlinNoise noise;
    
    // 为每个地形块生成一个高度
    int num_tiles = g_terrain_grid_size * g_terrain_grid_size;
    g_terrain_heights.resize(num_tiles);

    // 噪声参数
    float scale = 0.05f;  // 控制噪声频率（降低以适应更大的网格）
    float amplitude = 5.0f;  // 控制高度变化幅度
    float base_height = 2.0f;  // 基础高度

    for (int z = 0; z < g_terrain_grid_size; ++z)
    {
        for (int x = 0; x < g_terrain_grid_size; ++x)
        {
            int index = z * g_terrain_grid_size + x;
            
            // 计算噪声值
            double noise_value = noise.noise(x * scale, z * scale, 0.0);
            
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
            
            g_terrain_heights[index] = height;
        }
    }
}

// 创建地面几何体（整个地形网格，渲染所有面）
void create_ground_geometry()
{
    std::vector<float> vertices;
    
    auto add_quad = [&](const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, const glm::vec3& normal)
    {
        // 第一个三角形
        vertices.insert(vertices.end(), {v1.x, v1.y, v1.z, normal.x, normal.y, normal.z, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {v2.x, v2.y, v2.z, normal.x, normal.y, normal.z, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {v3.x, v3.y, v3.z, normal.x, normal.y, normal.z, 1.0f, 1.0f});
        
        // 第二个三角形
        vertices.insert(vertices.end(), {v1.x, v1.y, v1.z, normal.x, normal.y, normal.z, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {v3.x, v3.y, v3.z, normal.x, normal.y, normal.z, 1.0f, 1.0f});
        vertices.insert(vertices.end(), {v4.x, v4.y, v4.z, normal.x, normal.y, normal.z, 0.0f, 1.0f});
    };

    // 生成地形网格的顶点（每个块渲染所有6个面）
    const float box_height = 1.0f; // 盒子的高度
    
    for (int z = 0; z < g_terrain_grid_size; ++z)
    {
        for (int x = 0; x < g_terrain_grid_size; ++x)
        {
            // 计算四个角的世界坐标
            float world_x1 = (x - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            float world_x2 = (x + 1 - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            float world_z1 = (z - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            float world_z2 = (z + 1 - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;

            // 获取这个块的高度
            int tile_idx = z * g_terrain_grid_size + x;
            float height_top = g_terrain_heights[tile_idx];
            float height_bottom = height_top - box_height;

            // 8个顶点（盒子的角）
            glm::vec3 v0(world_x1, height_bottom, world_z1); // 左下前
            glm::vec3 v1(world_x2, height_bottom, world_z1); // 右下前
            glm::vec3 v2(world_x2, height_bottom, world_z2); // 右下后
            glm::vec3 v3(world_x1, height_bottom, world_z2); // 左下后
            glm::vec3 v4(world_x1, height_top, world_z1);    // 左上前
            glm::vec3 v5(world_x2, height_top, world_z1);    // 右上前
            glm::vec3 v6(world_x2, height_top, world_z2);    // 右上后
            glm::vec3 v7(world_x1, height_top, world_z2);    // 左上后

            // 顶面 (+Y)
            add_quad(v4, v5, v6, v7, glm::vec3(0, 1, 0));
            
            // 底面 (-Y)
            add_quad(v3, v2, v1, v0, glm::vec3(0, -1, 0));
            
            // 前面 (-Z)
            add_quad(v0, v1, v5, v4, glm::vec3(0, 0, -1));
            
            // 后面 (+Z)
            add_quad(v2, v3, v7, v6, glm::vec3(0, 0, 1));
            
            // 左面 (-X)
            add_quad(v3, v0, v4, v7, glm::vec3(-1, 0, 0));
            
            // 右面 (+X)
            add_quad(v1, v2, v6, v5, glm::vec3(1, 0, 0));
        }
    }

    glGenVertexArrays(1, &g_ground_vao);
    glGenBuffers(1, &g_ground_vbo);

    glBindVertexArray(g_ground_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_ground_vbo);
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
}

// 创建胶囊体几何体
void create_capsule_geometry(float radius, float half_height)
{
    std::vector<float> vertices;
    const int          segments = 20; // 圆周分段数
    const int          rings    = 10; // 半球的环数
    const float        PI       = 3.14159265f;

    auto add_vertex = [&](float x, float y, float z, float nx, float ny, float nz) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(nx);
        vertices.push_back(ny);
        vertices.push_back(nz);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
    };

    // 1. 圆柱体侧面（使用三角形）
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (float)i / segments * 2.0f * PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * PI;

        float x1 = radius * cosf(angle1);
        float z1 = radius * sinf(angle1);
        float x2 = radius * cosf(angle2);
        float z2 = radius * sinf(angle2);

        float nx1 = cosf(angle1);
        float nz1 = sinf(angle1);
        float nx2 = cosf(angle2);
        float nz2 = sinf(angle2);

        // 第一个三角形
        add_vertex(x1, -half_height, z1, nx1, 0, nz1);
        add_vertex(x1, half_height, z1, nx1, 0, nz1);
        add_vertex(x2, half_height, z2, nx2, 0, nz2);

        // 第二个三角形
        add_vertex(x1, -half_height, z1, nx1, 0, nz1);
        add_vertex(x2, half_height, z2, nx2, 0, nz2);
        add_vertex(x2, -half_height, z2, nx2, 0, nz2);
    }

    // 2. 顶部半球
    for (int j = 0; j < rings; ++j)
    {
        float phi1 = (float)j / rings * PI / 2.0f;
        float phi2 = (float)(j + 1) / rings * PI / 2.0f;

        for (int i = 0; i < segments; ++i)
        {
            float theta1 = (float)i / segments * 2.0f * PI;
            float theta2 = (float)(i + 1) / segments * 2.0f * PI;

            // 计算四个顶点
            float x1 = radius * cosf(phi1) * cosf(theta1);
            float y1 = radius * sinf(phi1) + half_height;
            float z1 = radius * cosf(phi1) * sinf(theta1);

            float x2 = radius * cosf(phi1) * cosf(theta2);
            float y2 = radius * sinf(phi1) + half_height;
            float z2 = radius * cosf(phi1) * sinf(theta2);

            float x3 = radius * cosf(phi2) * cosf(theta2);
            float y3 = radius * sinf(phi2) + half_height;
            float z3 = radius * cosf(phi2) * sinf(theta2);

            float x4 = radius * cosf(phi2) * cosf(theta1);
            float y4 = radius * sinf(phi2) + half_height;
            float z4 = radius * cosf(phi2) * sinf(theta1);

            // 法线（球面法线）
            glm::vec3 n1 = glm::normalize(glm::vec3(x1, y1 - half_height, z1));
            glm::vec3 n2 = glm::normalize(glm::vec3(x2, y2 - half_height, z2));
            glm::vec3 n3 = glm::normalize(glm::vec3(x3, y3 - half_height, z3));
            glm::vec3 n4 = glm::normalize(glm::vec3(x4, y4 - half_height, z4));

            // 第一个三角形
            add_vertex(x1, y1, z1, n1.x, n1.y, n1.z);
            add_vertex(x2, y2, z2, n2.x, n2.y, n2.z);
            add_vertex(x3, y3, z3, n3.x, n3.y, n3.z);

            // 第二个三角形
            add_vertex(x1, y1, z1, n1.x, n1.y, n1.z);
            add_vertex(x3, y3, z3, n3.x, n3.y, n3.z);
            add_vertex(x4, y4, z4, n4.x, n4.y, n4.z);
        }
    }

    // 3. 底部半球
    for (int j = 0; j < rings; ++j)
    {
        float phi1 = (float)j / rings * PI / 2.0f;
        float phi2 = (float)(j + 1) / rings * PI / 2.0f;

        for (int i = 0; i < segments; ++i)
        {
            float theta1 = (float)i / segments * 2.0f * PI;
            float theta2 = (float)(i + 1) / segments * 2.0f * PI;

            // 计算四个顶点（注意 y 坐标是负的）
            float x1 = radius * cosf(phi1) * cosf(theta1);
            float y1 = -radius * sinf(phi1) - half_height;
            float z1 = radius * cosf(phi1) * sinf(theta1);

            float x2 = radius * cosf(phi1) * cosf(theta2);
            float y2 = -radius * sinf(phi1) - half_height;
            float z2 = radius * cosf(phi1) * sinf(theta2);

            float x3 = radius * cosf(phi2) * cosf(theta2);
            float y3 = -radius * sinf(phi2) - half_height;
            float z3 = radius * cosf(phi2) * sinf(theta2);

            float x4 = radius * cosf(phi2) * cosf(theta1);
            float y4 = -radius * sinf(phi2) - half_height;
            float z4 = radius * cosf(phi2) * sinf(theta1);

            // 法线（球面法线，注意方向）
            glm::vec3 n1 = glm::normalize(glm::vec3(x1, y1 + half_height, z1));
            glm::vec3 n2 = glm::normalize(glm::vec3(x2, y2 + half_height, z2));
            glm::vec3 n3 = glm::normalize(glm::vec3(x3, y3 + half_height, z3));
            glm::vec3 n4 = glm::normalize(glm::vec3(x4, y4 + half_height, z4));

            // 第一个三角形（注意顶点顺序相反以保持正确的面朝向）
            add_vertex(x1, y1, z1, n1.x, n1.y, n1.z);
            add_vertex(x3, y3, z3, n3.x, n3.y, n3.z);
            add_vertex(x2, y2, z2, n2.x, n2.y, n2.z);

            // 第二个三角形
            add_vertex(x1, y1, z1, n1.x, n1.y, n1.z);
            add_vertex(x4, y4, z4, n4.x, n4.y, n4.z);
            add_vertex(x3, y3, z3, n3.x, n3.y, n3.z);
        }
    }

    g_capsule_vertex_count = vertices.size() / 8;

    glGenVertexArrays(1, &g_capsule_vao);
    glGenBuffers(1, &g_capsule_vbo);

    glBindVertexArray(g_capsule_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_capsule_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// 初始化 Jolt Physics
bool init_physics()
{
    // 注册所有 Jolt 的类型
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    // 创建临时内存分配器
    g_temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);

    // 创建作业系统
    g_job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

    // 设置层接口
    BPLayerInterfaceImpl*              broad_phase_layer_interface        = new BPLayerInterfaceImpl();
    ObjectVsBroadPhaseLayerFilterImpl* object_vs_broad_phase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();
    ObjectLayerPairFilterImpl*         object_layer_pair_filter           = new ObjectLayerPairFilterImpl();

    // 创建物理系统
    const uint cMaxBodies             = 1024;
    const uint cNumBodyMutexes        = 0;
    const uint cMaxBodyPairs          = 1024;
    const uint cMaxContactConstraints = 1024;

    g_physics_system = new PhysicsSystem();
    g_physics_system->Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        *broad_phase_layer_interface,
        *object_vs_broad_phase_layer_filter,
        *object_layer_pair_filter
    );

    // 设置联系监听器
    MyContactListener* contact_listener = new MyContactListener();
    g_physics_system->SetContactListener(contact_listener);

    // 设置身体激活监听器
    MyBodyActivationListener* body_activation_listener = new MyBodyActivationListener();
    g_physics_system->SetBodyActivationListener(body_activation_listener);

    // 创建地形（为每个块创建一个盒子）
    BodyInterface& body_interface = g_physics_system->GetBodyInterface();

    // 每个地形块的一半尺寸
    float half_tile_size = g_terrain_tile_size / 2.0f;
    float half_height    = 0.5f; // 每个块的厚度

    // 为每个地形块创建物理盒子
    for (int z = 0; z < g_terrain_grid_size; ++z)
    {
        for (int x = 0; x < g_terrain_grid_size; ++x)
        {
            // 获取这个块的高度
            int   tile_idx = z * g_terrain_grid_size + x;
            float height   = g_terrain_heights[tile_idx];

            // 计算这个块的中心位置
            float world_x = (x + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            float world_z = (z + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;

            // 创建盒子形状
            BoxShapeSettings           box_shape_settings(Vec3(half_tile_size, half_height, half_tile_size));
            ShapeSettings::ShapeResult box_shape_result = box_shape_settings.Create();
            ShapeRefC                  box_shape        = box_shape_result.Get();

            // 创建刚体（中心在块的中间，Y轴在块的顶部下方 half_height 处）
            BodyCreationSettings tile_settings(box_shape, RVec3(world_x, height - half_height, world_z), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
            Body*                tile = body_interface.CreateBody(tile_settings);
            body_interface.AddBody(tile->GetID(), EActivation::DontActivate);
        }
    }

    // 创建角色控制器
    Ref<CharacterSettings> character_settings = new CharacterSettings();
    character_settings->mMaxSlopeAngle        = DegreesToRadians(45.0f);
    character_settings->mLayer                = Layers::MOVING;
    character_settings->mShape                = new CapsuleShape(0.4f, 0.3f); // 半高度0.4m，半径0.3m（总高度1.4m）
    character_settings->mFriction             = 0.5f;
    character_settings->mSupportingVolume     = Plane(Vec3::sAxisY(), -0.4f);

    g_character = new Character(character_settings, RVec3(0, 3, 0), Quat::sIdentity(), 0, g_physics_system);
    g_character->AddToPhysicsSystem(EActivation::Activate);

    cout << "Jolt Physics 初始化成功！" << endl;
    return true;
}

// 更新物理
void update_physics(float delta_time)
{
    if (!g_physics_system || !g_character)
        return;

    // 计算摄像机角度（供移动和摄像机位置计算使用）
    float yaw_rad = glm::radians(g_camera_yaw);
    float pitch_rad = glm::radians(g_camera_pitch);
    
    // 处理角色输入
    const float move_speed = 5.0f;
    
    // 计算摄像机的前方和右方向（基于 yaw 角，忽略 pitch）
    // 摄像机看向目标的方向 = -(offset的水平分量)
    Vec3  camera_forward(cosf(yaw_rad), 0, sinf(yaw_rad)); // 摄像机前方
    Vec3  camera_right(-sinf(yaw_rad), 0, cosf(yaw_rad));    // 摄像机右方 (up × forward)

    // 根据输入计算相对于摄像机的移动方向
    Vec3 movement_direction(0, 0, 0);
    if (g_key_w)
        movement_direction += camera_forward;
    if (g_key_s)
        movement_direction -= camera_forward;
    if (g_key_a)
        movement_direction -= camera_right;
    if (g_key_d)
        movement_direction += camera_right;

    // 归一化移动方向
    float length = movement_direction.Length();
    if (length > 0.001f)
    {
        movement_direction = movement_direction / length;
    }

    // 设置角色速度
    Vec3 current_velocity = g_character->GetLinearVelocity();
    Vec3 desired_velocity = movement_direction * move_speed;
    desired_velocity.SetY(current_velocity.GetY()); // 保持垂直速度

    // 跳跃
    if (g_key_space && g_character->GetGroundState() == Character::EGroundState::OnGround)
    {
        desired_velocity.SetY(5.5f); // 跳跃速度（可跳约1.5m高）
    }

    g_character->SetLinearVelocity(desired_velocity);

    // 更新物理世界
    const int cCollisionSteps = 1;
    g_physics_system->Update(delta_time, cCollisionSteps, g_temp_allocator, g_job_system);

    // 更新角色位置（在物理更新之后）
    g_character->PostSimulation(0.55f);

    // 更新摄像机跟随角色
    RVec3 char_pos  = g_character->GetPosition();
    g_camera_target = glm::vec3((float)char_pos.GetX(), (float)char_pos.GetY() + 1.0f, (float)char_pos.GetZ());

    // 根据偏航角和俯仰角计算摄像机位置（使用之前计算的角度）
    glm::vec3 camera_offset;
    camera_offset.x = g_camera_distance * cos(pitch_rad) * cos(yaw_rad);
    camera_offset.y = g_camera_distance * sin(pitch_rad);
    camera_offset.z = g_camera_distance * cos(pitch_rad) * sin(yaw_rad);

    g_camera_pos = g_camera_target - camera_offset;
}

// 渲染场景
void render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 使用着色器程序
    glUseProgram(g_shader_program);

    // 设置矩阵
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 view       = glm::lookAt(g_camera_pos, g_camera_target, glm::vec3(0, 1, 0));

    GLint proj_loc         = glGetUniformLocation(g_shader_program, "projection");
    GLint view_loc         = glGetUniformLocation(g_shader_program, "view");
    GLint model_loc        = glGetUniformLocation(g_shader_program, "model");
    GLint light_pos_loc    = glGetUniformLocation(g_shader_program, "lightPos");
    GLint view_pos_loc     = glGetUniformLocation(g_shader_program, "viewPos");
    GLint object_color_loc = glGetUniformLocation(g_shader_program, "objectColor");
    GLint use_texture_loc  = glGetUniformLocation(g_shader_program, "useTexture");
    GLint use_alpha_loc    = glGetUniformLocation(g_shader_program, "useAlpha");
    GLint alpha_loc        = glGetUniformLocation(g_shader_program, "alpha");

    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniform3f(light_pos_loc, 10.0f, 20.0f, 10.0f);
    glUniform3f(view_pos_loc, g_camera_pos.x, g_camera_pos.y, g_camera_pos.z);

    // 绘制地形
    glm::mat4 ground_model = glm::mat4(1.0f);
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(ground_model));
    glUniform3f(object_color_loc, 1.0f, 1.0f, 1.0f);
    glUniform1i(use_texture_loc, 1);
    glUniform1i(use_alpha_loc, 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_ground_texture);
    glUniform1i(glGetUniformLocation(g_shader_program, "textureSampler"), 0);
    
    glBindVertexArray(g_ground_vao);
    // 绘制整个地形网格 (15x15 块，每块 6 个面，每面 2 个三角形 = 6 个顶点，总共 36 个顶点/块)
    int total_vertices = g_terrain_grid_size * g_terrain_grid_size * 36;
    glDrawArrays(GL_TRIANGLES, 0, total_vertices);

    // 绘制角色
    if (g_character)
    {
        RVec3     char_pos      = g_character->GetPosition();
        glm::mat4 capsule_model = glm::translate(glm::mat4(1.0f), glm::vec3((float)char_pos.GetX(), (float)char_pos.GetY(), (float)char_pos.GetZ()));

        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(capsule_model));
        glUniform3f(object_color_loc, 0.4f, 0.4f, 1.0f);
        glUniform1i(use_texture_loc, 0);
        glUniform1i(use_alpha_loc, 0);

        glBindVertexArray(g_capsule_vao);
        glDrawArrays(GL_TRIANGLES, 0, g_capsule_vertex_count);
    }

    glBindVertexArray(0);
    glfwSwapBuffers(g_window);
}

int main()
{
    cout << "=== Jolt Physics 角色控制器示例 ===" << endl;
    cout << "控制说明：" << endl;
    cout << "  WASD       - 移动角色" << endl;
    cout << "  空格       - 跳跃" << endl;
    cout << "  P          - 切换鼠标捕获（旋转摄像机）" << endl;
    cout << "  鼠标滚轮   - 调整摄像机距离" << endl;
    cout << "  ESC        - 退出" << endl;
    cout << "====================================" << endl;

    // 初始化 GLFW
    if (!glfwInit())
    {
        cerr << "Failed to initialize GLFW" << endl;
        return -1;
    }

    // 设置 OpenGL 版本（使用兼容模式以支持旧的立即模式 API）
#ifdef __APPLE__
    // macOS 只支持 Core Profile 从 3.2 开始，但我们可以使用 2.1 以获得兼容模式
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    // 创建窗口
    g_window = glfwCreateWindow(800, 600, "Jolt Character Controller Example", nullptr, nullptr);
    if (!g_window)
    {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(g_window);
    glfwSetKeyCallback(g_window, key_callback);
    glfwSetCursorPosCallback(g_window, mouse_callback);
    glfwSetScrollCallback(g_window, scroll_callback);
    glfwSwapInterval(1); // 启用垂直同步

    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Failed to initialize GLAD" << endl;
        glfwDestroyWindow(g_window);
        glfwTerminate();
        return -1;
    }

    cout << "OpenGL 版本: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL 版本: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    // 设置 OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // 天蓝色背景

    // 创建着色器程序
    g_shader_program = create_shader_program();
    
    // 生成地形高度数据
    generate_terrain_heights();
    
    // 创建几何体
    create_ground_geometry();
    create_capsule_geometry(0.3f, 0.4f); // 半径0.3m，半高度0.4m（总高度1.4m）
    
    // 创建地面纹理
    g_ground_texture = create_checkerboard_texture();

    // 初始化物理
    if (!init_physics())
    {
        cerr << "Failed to initialize physics" << endl;
        glfwDestroyWindow(g_window);
        glfwTerminate();
        return -1;
    }

    // 主循环
    double last_time = glfwGetTime();
    while (!glfwWindowShouldClose(g_window))
    {
        double current_time = glfwGetTime();
        float  delta_time   = (float)(current_time - last_time);
        last_time           = current_time;

        // 限制时间步长
        if (delta_time > 0.1f)
            delta_time = 0.1f;

        glfwPollEvents();
        update_physics(delta_time);
        render();
    }

    // 清理 OpenGL 资源
    if (g_ground_vao)
        glDeleteVertexArrays(1, &g_ground_vao);
    if (g_ground_vbo)
        glDeleteBuffers(1, &g_ground_vbo);
    if (g_capsule_vao)
        glDeleteVertexArrays(1, &g_capsule_vao);
    if (g_capsule_vbo)
        glDeleteBuffers(1, &g_capsule_vbo);
    if (g_ground_texture)
        glDeleteTextures(1, &g_ground_texture);
    if (g_shader_program)
        glDeleteProgram(g_shader_program);

    // 清理物理资源
    if (g_character)
    {
        g_character->RemoveFromPhysicsSystem();
        delete g_character;
    }

    if (g_physics_system)
    {
        delete g_physics_system;
    }

    if (g_job_system)
    {
        delete g_job_system;
    }

    if (g_temp_allocator)
    {
        delete g_temp_allocator;
    }

    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;

    glfwDestroyWindow(g_window);
    glfwTerminate();

    cout << "程序正常退出" << endl;
    return 0;
}
