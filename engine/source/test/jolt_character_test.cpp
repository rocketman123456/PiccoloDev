// Jolt Physics Character Controller Example
// 使用 GLFW 和 OpenGL 3.3 Core Profile 渲染一个在平坦地面上移动的胶囊体角色

// clang-format off
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
// clang-format on

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <map>
#include <set>

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
int    g_ground_vertex_count  = 0;
GLuint g_capsule_vao          = 0;
GLuint g_capsule_vbo          = 0;
int    g_capsule_vertex_count = 0;

// 调试渲染资源
GLuint g_debug_vao            = 0;
GLuint g_debug_vbo            = 0;
int    g_debug_vertex_count   = 0;
bool   g_debug_collision_mesh = false; // 是否显示碰撞网格调试

// 地面碰撞体实心渲染资源
GLuint g_ground_collision_vao = 0;
GLuint g_ground_collision_vbo = 0;
int    g_ground_collision_vertex_count = 0;
bool   g_debug_ground_collision_solid = false; // 是否显示地面碰撞体实心渲染

// 角色碰撞体调试渲染资源
GLuint g_character_debug_vao  = 0;
GLuint g_character_debug_vbo  = 0;
int    g_character_debug_vertex_count = 0;
bool   g_debug_character_collision = false; // 是否显示角色碰撞体调试

// 动态地面物理系统
const int g_physics_radius = 5; // 20x20区域，半径10
std::map<std::pair<int, int>, BodyID> g_active_physics_bodies; // 当前激活的物理体
std::pair<int, int> g_last_character_grid_pos = {-999, -999}; // 上次角色所在的网格位置

// 自动地形更新系统
const int g_terrain_radius = 20; // 100x100区域，半径50
std::map<std::pair<int, int>, float> g_generated_terrain; // 已生成的地形高度
std::pair<int, int> g_last_terrain_center = {-999, -999}; // 上次地形中心位置
const int g_terrain_update_threshold = 10; // 当角色距离地形中心超过10格时更新

// 地形参数
const int          g_terrain_grid_size = 200;   // 50 x 50 地形块
const float        g_terrain_tile_size = 1.0f; // 每块 1m x 1m
std::vector<float> g_terrain_heights;          // 存储每个网格点的高度

// 摄像机参数
glm::vec3 g_camera_pos(0, 5, 10);
glm::vec3 g_camera_target(0, 1, 0);
float     g_camera_yaw      = -90.0f; // 偏航角（左右旋转）
float     g_camera_pitch    = -20.0f; // 俯仰角（上下旋转）
float     g_camera_distance = 5.0f;   // 摄像机距离角色的距离（适应100x100地形）
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

// 函数声明
void update_ground_collision_solid_geometry();
void update_debug_collision_geometry();
void update_terrain_around_character();
float generate_terrain_height(int grid_x, int grid_z);
void update_ground_geometry();

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
    
    // O 键切换调试碰撞网格
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_debug_collision_mesh = !g_debug_collision_mesh;
        cout << "调试碰撞网格: " << (g_debug_collision_mesh ? "开启" : "关闭") << endl;
        if (g_debug_collision_mesh)
        {
            // 立即创建调试线框几何体
            update_debug_collision_geometry();
            cout << "绿色线框顶点数: " << g_debug_vertex_count << endl;
            cout << "VAO ID: " << g_debug_vao << ", VBO ID: " << g_debug_vbo << endl;
        }
        else
        {
            // 关闭时删除几何体
            if (g_debug_vao != 0)
            {
                glDeleteVertexArrays(1, &g_debug_vao);
                glDeleteBuffers(1, &g_debug_vbo);
                g_debug_vao = 0;
                g_debug_vbo = 0;
                g_debug_vertex_count = 0;
            }
        }
    }
    
    // C 键切换角色碰撞体调试
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        g_debug_character_collision = !g_debug_character_collision;
        cout << "角色碰撞体调试: " << (g_debug_character_collision ? "开启" : "关闭") << endl;
    }

    // B 键切换地面碰撞体实心渲染
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
    {
        g_debug_ground_collision_solid = !g_debug_ground_collision_solid;
        cout << "地面碰撞体实心渲染: " << (g_debug_ground_collision_solid ? "开启" : "关闭") << endl;
        if (g_debug_ground_collision_solid)
        {
            // 立即创建蓝色实心几何体
            update_ground_collision_solid_geometry();
            cout << "蓝色实心盒子顶点数: " << g_ground_collision_vertex_count << endl;
            cout << "VAO ID: " << g_ground_collision_vao << ", VBO ID: " << g_ground_collision_vbo << endl;
        }
        else
        {
            // 关闭时删除几何体
            if (g_ground_collision_vao != 0)
            {
                glDeleteVertexArrays(1, &g_ground_collision_vao);
                glDeleteBuffers(1, &g_ground_collision_vbo);
                g_ground_collision_vao = 0;
                g_ground_collision_vbo = 0;
                g_ground_collision_vertex_count = 0;
            }
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

    double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }

    double lerp(double t, double a, double b) { return a + t * (b - a); }

    double grad(int hash, double x, double y, double z)
    {
        int    h = hash & 15;
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

        int A  = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B  = p[X + 1] + Y;
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
};

// 生成地形高度数据（使用 Perlin 噪声，量化为整数高度）
void generate_terrain_heights()
{
    PerlinNoise noise;

    // 为每个地形块生成一个高度
    int num_tiles = g_terrain_grid_size * g_terrain_grid_size;
    g_terrain_heights.resize(num_tiles);

    // 噪声参数
    float scale       = 0.05f; // 控制噪声频率（降低以适应更大的网格）
    float amplitude   = 5.0f;  // 控制高度变化幅度
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

// 视锥剔除：检查地形块是否在摄像机视野内
bool is_terrain_tile_visible(int x, int z, const glm::mat4& view_proj_matrix)
{
    // 计算地形块的世界坐标
    float world_x1 = (x - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
    float world_x2 = (x + 1 - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
    float world_z1 = (z - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
    float world_z2 = (z + 1 - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;

    // 获取地形块高度
    int   tile_idx      = z * g_terrain_grid_size + x;
    float height_top    = g_terrain_heights[tile_idx];
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
        if (clip_pos.x >= -clip_pos.w && clip_pos.x <= clip_pos.w && clip_pos.y >= -clip_pos.w && clip_pos.y <= clip_pos.w && clip_pos.z >= -clip_pos.w &&
            clip_pos.z <= clip_pos.w)
        {
            all_outside = false;
            break;
        }
    }

    return !all_outside;
}

// 超高效的地面几何体创建（批量处理，减少内存操作）
void create_ground_geometry()
{
    cout << "开始创建地形几何体..." << endl;

    // 估算实际需要的顶点数量（更精确的估算）
    int       estimated_vertices = 0;
    const int total_tiles        = g_terrain_grid_size * g_terrain_grid_size;

    // 第一遍：计算实际需要的顶点数量
    for (int z = 0; z < g_terrain_grid_size; ++z)
    {
        for (int x = 0; x < g_terrain_grid_size; ++x)
        {
            int   tile_idx   = z * g_terrain_grid_size + x;
            float height_top = g_terrain_heights[tile_idx];

            // 每个块至少有顶面（6个顶点）
            estimated_vertices += 6;

            // 检查侧面是否需要渲染
            if (z == 0 || g_terrain_heights[(z - 1) * g_terrain_grid_size + x] < height_top)
                estimated_vertices += 6; // 前面
            if (z == g_terrain_grid_size - 1 || g_terrain_heights[(z + 1) * g_terrain_grid_size + x] < height_top)
                estimated_vertices += 6; // 后面
            if (x == 0 || g_terrain_heights[z * g_terrain_grid_size + (x - 1)] < height_top)
                estimated_vertices += 6; // 左面
            if (x == g_terrain_grid_size - 1 || g_terrain_heights[z * g_terrain_grid_size + (x + 1)] < height_top)
                estimated_vertices += 6; // 右面
        }
    }

    // 预分配精确的内存
    std::vector<float> vertices;
    vertices.reserve(estimated_vertices * 8); // 每个顶点8个float

    cout << "预分配内存: " << estimated_vertices << " 个顶点" << endl;

    // 超高效的添加四边形函数（直接操作内存）
    auto add_quad_ultra_fast =
        [&](float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4, float nx, float ny, float nz
        ) {
            // 直接添加18个float（6个顶点 * 3个float/顶点）
            vertices.insert(
                vertices.end(),
                {
                    x1, y1, z1, nx, ny, nz, 0.0f, 0.0f, // 顶点1
                    x2, y2, z2, nx, ny, nz, 1.0f, 0.0f, // 顶点2
                    x3, y3, z3, nx, ny, nz, 1.0f, 1.0f, // 顶点3
                    x1, y1, z1, nx, ny, nz, 0.0f, 0.0f, // 顶点4
                    x3, y3, z3, nx, ny, nz, 1.0f, 1.0f, // 顶点5
                    x4, y4, z4, nx, ny, nz, 0.0f, 1.0f  // 顶点6
                }
            );
        };

    const float box_height = 1.0f;
    const float half_size  = g_terrain_grid_size * 0.5f;

    // 批量处理：减少重复计算，添加进度显示
    cout << "开始生成顶点数据..." << endl;
    int processed_tiles = 0;

    for (int z = 0; z < g_terrain_grid_size; ++z)
    {
        float world_z1 = (z - half_size) * g_terrain_tile_size;
        float world_z2 = (z + 1 - half_size) * g_terrain_tile_size;

        for (int x = 0; x < g_terrain_grid_size; ++x)
        {
            float world_x1 = (x - half_size) * g_terrain_tile_size;
            float world_x2 = (x + 1 - half_size) * g_terrain_tile_size;

            int   tile_idx      = z * g_terrain_grid_size + x;
            float height_top    = g_terrain_heights[tile_idx];
            float height_bottom = height_top - box_height;

            // 只渲染顶面和侧面（底面通常不可见）
            // 顶面 (+Y)
            add_quad_ultra_fast(
                world_x1, height_top, world_z1, world_x2, height_top, world_z1, world_x2, height_top, world_z2, world_x1, height_top, world_z2, 0.0f, 1.0f, 0.0f
            );

            // 前面 (-Z) - 只在高度变化时渲染
            if (z == 0 || g_terrain_heights[(z - 1) * g_terrain_grid_size + x] < height_top)
            {
                add_quad_ultra_fast(
                    world_x1,
                    height_bottom,
                    world_z1,
                    world_x2,
                    height_bottom,
                    world_z1,
                    world_x2,
                    height_top,
                    world_z1,
                    world_x1,
                    height_top,
                    world_z1,
                    0.0f,
                    0.0f,
                    -1.0f
                );
            }

            // 后面 (+Z) - 只在高度变化时渲染
            if (z == g_terrain_grid_size - 1 || g_terrain_heights[(z + 1) * g_terrain_grid_size + x] < height_top)
            {
                add_quad_ultra_fast(
                    world_x2,
                    height_bottom,
                    world_z2,
                    world_x1,
                    height_bottom,
                    world_z2,
                    world_x1,
                    height_top,
                    world_z2,
                    world_x2,
                    height_top,
                    world_z2,
                    0.0f,
                    0.0f,
                    1.0f
                );
            }

            // 左面 (-X) - 只在高度变化时渲染
            if (x == 0 || g_terrain_heights[z * g_terrain_grid_size + (x - 1)] < height_top)
            {
                add_quad_ultra_fast(
                    world_x1,
                    height_bottom,
                    world_z2,
                    world_x1,
                    height_bottom,
                    world_z1,
                    world_x1,
                    height_top,
                    world_z1,
                    world_x1,
                    height_top,
                    world_z2,
                    -1.0f,
                    0.0f,
                    0.0f
                );
            }

            // 右面 (+X) - 只在高度变化时渲染
            if (x == g_terrain_grid_size - 1 || g_terrain_heights[z * g_terrain_grid_size + (x + 1)] < height_top)
            {
                add_quad_ultra_fast(
                    world_x2,
                    height_bottom,
                    world_z1,
                    world_x2,
                    height_bottom,
                    world_z2,
                    world_x2,
                    height_top,
                    world_z2,
                    world_x2,
                    height_top,
                    world_z1,
                    1.0f,
                    0.0f,
                    0.0f
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

    cout << "创建OpenGL缓冲区..." << endl;
    glGenVertexArrays(1, &g_ground_vao);
    glGenBuffers(1, &g_ground_vbo);

    cout << "上传顶点数据到GPU..." << endl;
    glBindVertexArray(g_ground_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_ground_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    cout << "设置顶点属性..." << endl;
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

    // 设置顶点数量（每个顶点8个float）
    g_ground_vertex_count = vertices.size() / 8;

    cout << "地形几何体创建完成: " << g_ground_vertex_count << " 个顶点" << endl;
    cout << "内存使用: " << (vertices.size() * sizeof(float) / 1024.0f / 1024.0f) << " MB" << endl;
}

// 创建调试碰撞网格几何体（线框模式）
void create_debug_collision_geometry()
{
    cout << "创建调试碰撞网格几何体（动态区域）..." << endl;
    
    std::vector<float> vertices;
    
    // 为当前激活的物理体创建线框盒子
    const float half_tile_size = g_terrain_tile_size / 2.0f;
    const float half_height = 0.5f;
    
    for (const auto& body_pair : g_active_physics_bodies)
    {
        int grid_x = body_pair.first.first;
        int grid_z = body_pair.first.second;
        
        // 获取这个块的高度（从动态地形数据）
        auto terrain_it = g_generated_terrain.find({grid_x, grid_z});
        if (terrain_it == g_generated_terrain.end())
            continue; // 地形数据不存在，跳过
        
        float height = terrain_it->second;
        
        // 计算这个块的中心位置
        float world_x = (grid_x + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
        float world_z = (grid_z + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
        
        // 计算盒子的8个角点（与物理系统位置一致）
        // 物理体中心在 height - half_height，所以：
        // 物理体底部 = (height - half_height) - half_height = height - 2*half_height
        // 物理体顶部 = (height - half_height) + half_height = height
        float physics_center_y = height - half_height;
        float x1 = world_x - half_tile_size;
        float x2 = world_x + half_tile_size;
        float y1 = physics_center_y - half_height; // 物理体底部
        float y2 = physics_center_y + half_height; // 物理体顶部
        float z1 = world_z - half_tile_size;
        float z2 = world_z + half_tile_size;
        
        // 添加盒子的12条边（每条边2个顶点）
        // 底面4条边
        vertices.insert(vertices.end(), {x1, y1, z1, x2, y1, z1}); // 底面前边
        vertices.insert(vertices.end(), {x2, y1, z1, x2, y1, z2}); // 底面右边
        vertices.insert(vertices.end(), {x2, y1, z2, x1, y1, z2}); // 底面后边
        vertices.insert(vertices.end(), {x1, y1, z2, x1, y1, z1}); // 底面左边
        
        // 顶面4条边
        vertices.insert(vertices.end(), {x1, y2, z1, x2, y2, z1}); // 顶面前边
        vertices.insert(vertices.end(), {x2, y2, z1, x2, y2, z2}); // 顶面右边
        vertices.insert(vertices.end(), {x2, y2, z2, x1, y2, z2}); // 顶面后边
        vertices.insert(vertices.end(), {x1, y2, z2, x1, y2, z1}); // 顶面左边
        
        // 4条垂直边
        vertices.insert(vertices.end(), {x1, y1, z1, x1, y2, z1}); // 前左垂直边
        vertices.insert(vertices.end(), {x2, y1, z1, x2, y2, z1}); // 前右垂直边
        vertices.insert(vertices.end(), {x2, y1, z2, x2, y2, z2}); // 后右垂直边
        vertices.insert(vertices.end(), {x1, y1, z2, x1, y2, z2}); // 后左垂直边
    }
    
    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &g_debug_vao);
    glGenBuffers(1, &g_debug_vbo);
    
    glBindVertexArray(g_debug_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_debug_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // 只设置位置属性（线框不需要法线和纹理坐标）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    g_debug_vertex_count = vertices.size() / 3; // 每个顶点3个float
    
    cout << "调试碰撞网格创建完成: " << g_debug_vertex_count << " 个顶点" << endl;
    cout << "激活物理体数量: " << g_active_physics_bodies.size() << endl;
}

// 创建角色碰撞体调试几何体（线框模式）
void create_character_debug_geometry()
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
    
    // 胶囊体上半球
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (float)i / segments * 2.0f * M_PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * M_PI;
        
        for (int j = 0; j < segments / 2; ++j)
        {
            float phi1 = (float)j / (segments / 2) * M_PI / 2.0f;
            float phi2 = (float)(j + 1) / (segments / 2) * M_PI / 2.0f;
            
            float x1 = capsule_radius * cosf(phi1) * cosf(angle1);
            float y1 = capsule_radius * sinf(phi1) + capsule_half_height;
            float z1 = capsule_radius * cosf(phi1) * sinf(angle1);
            
            float x2 = capsule_radius * cosf(phi1) * cosf(angle2);
            float y2 = capsule_radius * sinf(phi1) + capsule_half_height;
            float z2 = capsule_radius * cosf(phi1) * sinf(angle2);
            
            vertices.insert(vertices.end(), {x1, y1, z1, x2, y2, z2});
        }
    }
    
    // 胶囊体下半球
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (float)i / segments * 2.0f * M_PI;
        float angle2 = (float)(i + 1) / segments * 2.0f * M_PI;
        
        for (int j = 0; j < segments / 2; ++j)
        {
            float phi1 = (float)j / (segments / 2) * M_PI / 2.0f;
            float phi2 = (float)(j + 1) / (segments / 2) * M_PI / 2.0f;
            
            float x1 = capsule_radius * cosf(phi1) * cosf(angle1);
            float y1 = -capsule_radius * sinf(phi1) - capsule_half_height;
            float z1 = capsule_radius * cosf(phi1) * sinf(angle1);
            
            float x2 = capsule_radius * cosf(phi1) * cosf(angle2);
            float y2 = -capsule_radius * sinf(phi1) - capsule_half_height;
            float z2 = capsule_radius * cosf(phi1) * sinf(angle2);
            
            vertices.insert(vertices.end(), {x1, y1, z1, x2, y2, z2});
        }
    }
    
    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &g_character_debug_vao);
    glGenBuffers(1, &g_character_debug_vbo);
    
    glBindVertexArray(g_character_debug_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_character_debug_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // 只设置位置属性（线框不需要法线和纹理坐标）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    g_character_debug_vertex_count = vertices.size() / 3; // 每个顶点3个float
    
    cout << "角色碰撞体调试几何体创建完成: " << g_character_debug_vertex_count << " 个顶点" << endl;
}

// 创建地面碰撞体实心几何体（蓝色实心盒子）- 只创建当前激活的物理体
void create_ground_collision_solid_geometry()
{
    cout << "创建地面碰撞体实心几何体（动态区域）..." << endl;

    std::vector<float> vertices;

    // 为当前激活的物理体创建实心盒子
    const float half_tile_size = g_terrain_tile_size / 2.0f;
    const float half_height = 0.5f;

    for (const auto& body_pair : g_active_physics_bodies)
    {
        int grid_x = body_pair.first.first;
        int grid_z = body_pair.first.second;
        
        // 获取这个块的高度（从动态地形数据）
        auto terrain_it = g_generated_terrain.find({grid_x, grid_z});
        if (terrain_it == g_generated_terrain.end())
            continue; // 地形数据不存在，跳过
        
        float height = terrain_it->second;

        // 计算这个块的中心位置
        float world_x = (grid_x + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
        float world_z = (grid_z + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;

        // 计算盒子的8个角点（与物理系统位置一致）
        float physics_center_y = height - half_height;
        float x1 = world_x - half_tile_size;
        float x2 = world_x + half_tile_size;
        float y1 = physics_center_y - half_height; // 物理体底部
        float y2 = physics_center_y + half_height; // 物理体顶部
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

    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &g_ground_collision_vao);
    glGenBuffers(1, &g_ground_collision_vbo);

    glBindVertexArray(g_ground_collision_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_ground_collision_vbo);
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

    g_ground_collision_vertex_count = vertices.size() / 8; // 每个顶点8个float

    cout << "地面碰撞体实心几何体创建完成: " << g_ground_collision_vertex_count << " 个顶点" << endl;
    cout << "激活物理体数量: " << g_active_physics_bodies.size() << endl;
}

// 将世界坐标转换为网格坐标
std::pair<int, int> world_to_grid(float world_x, float world_z)
{
    int grid_x = (int)floor(world_x + g_terrain_grid_size * 0.5f);
    int grid_z = (int)floor(world_z + g_terrain_grid_size * 0.5f);
    return {grid_x, grid_z};
}

// 检查网格坐标是否有效
bool is_valid_grid_pos(int grid_x, int grid_z)
{
    return grid_x >= 0 && grid_x < g_terrain_grid_size && 
           grid_z >= 0 && grid_z < g_terrain_grid_size;
}

// 创建单个地面物理体
BodyID create_ground_physics_body(int grid_x, int grid_z, BodyInterface& body_interface)
{
    // 检查地形数据是否存在
    auto terrain_it = g_generated_terrain.find({grid_x, grid_z});
    if (terrain_it == g_generated_terrain.end())
        return BodyID(); // 地形数据不存在，不创建物理体
    
    float height = terrain_it->second;
    
    // 计算世界位置
    float world_x = (grid_x + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
    float world_z = (grid_z + 0.5f - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
    
    // 创建物理体
    float half_tile_size = g_terrain_tile_size / 2.0f;
    float half_height = 0.5f;
    
    BoxShapeSettings box_shape_settings(Vec3(half_tile_size, half_height, half_tile_size));
    ShapeSettings::ShapeResult box_shape_result = box_shape_settings.Create();
    ShapeRefC box_shape = box_shape_result.Get();
    
    BodyCreationSettings tile_settings(
        box_shape, 
        RVec3(world_x, height - half_height, world_z), 
        Quat::sIdentity(), 
        EMotionType::Static, 
        Layers::NON_MOVING
    );
    
    Body* tile = body_interface.CreateBody(tile_settings);
    body_interface.AddBody(tile->GetID(), EActivation::DontActivate);
    
    return tile->GetID();
}

// 移除单个地面物理体
void remove_ground_physics_body(BodyID body_id, BodyInterface& body_interface)
{
    if (body_id.IsInvalid())
        return;
    
    body_interface.RemoveBody(body_id);
    body_interface.DestroyBody(body_id);
}

// 更新动态地面物理系统
void update_dynamic_ground_physics()
{
    if (!g_character || !g_physics_system)
        return;
    
    // 获取角色当前位置
    RVec3 char_pos = g_character->GetPosition();
    auto current_grid_pos = world_to_grid((float)char_pos.GetX(), (float)char_pos.GetZ());
    
    // 检查是否需要更新
    if (current_grid_pos == g_last_character_grid_pos)
        return; // 角色没有移动到新的网格位置，不需要更新
    
    cout << "角色移动到网格位置: (" << current_grid_pos.first << ", " << current_grid_pos.second << ")" << endl;
    
    BodyInterface& body_interface = g_physics_system->GetBodyInterface();
    
    // 计算新的20x20区域（基于动态地形数据）
    std::set<std::pair<int, int>> new_region;
    for (int dx = -g_physics_radius; dx <= g_physics_radius; ++dx)
    {
        for (int dz = -g_physics_radius; dz <= g_physics_radius; ++dz)
        {
            int grid_x = current_grid_pos.first + dx;
            int grid_z = current_grid_pos.second + dz;
            
            // 检查地形数据是否存在
            if (g_generated_terrain.find({grid_x, grid_z}) != g_generated_terrain.end())
            {
                new_region.insert({grid_x, grid_z});
            }
        }
    }
    
    // 移除不再需要的物理体
    auto it = g_active_physics_bodies.begin();
    while (it != g_active_physics_bodies.end())
    {
        if (new_region.find(it->first) == new_region.end())
        {
            remove_ground_physics_body(it->second, body_interface);
            it = g_active_physics_bodies.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    // 添加新的物理体
    for (const auto& grid_pos : new_region)
    {
        if (g_active_physics_bodies.find(grid_pos) == g_active_physics_bodies.end())
        {
            BodyID new_body = create_ground_physics_body(grid_pos.first, grid_pos.second, body_interface);
            if (!new_body.IsInvalid())
            {
                g_active_physics_bodies[grid_pos] = new_body;
            }
        }
    }
    
    g_last_character_grid_pos = current_grid_pos;
    cout << "动态地面物理更新完成，当前激活物理体数量: " << g_active_physics_bodies.size() << endl;
    
    // 如果蓝色实心碰撞体正在显示，更新其几何体
    update_ground_collision_solid_geometry();
    
    // 如果绿色线框碰撞体正在显示，更新其几何体
    update_debug_collision_geometry();
}

// 更新蓝色实心碰撞体几何体
void update_ground_collision_solid_geometry()
{
    if (!g_debug_ground_collision_solid)
        return;
    
    // 删除旧的几何体
    if (g_ground_collision_vao != 0)
    {
        glDeleteVertexArrays(1, &g_ground_collision_vao);
        glDeleteBuffers(1, &g_ground_collision_vbo);
        g_ground_collision_vao = 0;
        g_ground_collision_vbo = 0;
        g_ground_collision_vertex_count = 0;
    }
    
    // 重新创建几何体
    create_ground_collision_solid_geometry();
}

// 更新调试线框碰撞体几何体
void update_debug_collision_geometry()
{
    if (!g_debug_collision_mesh)
        return;
    
    // 删除旧的几何体
    if (g_debug_vao != 0)
    {
        glDeleteVertexArrays(1, &g_debug_vao);
        glDeleteBuffers(1, &g_debug_vbo);
        g_debug_vao = 0;
        g_debug_vbo = 0;
        g_debug_vertex_count = 0;
    }
    
    // 重新创建几何体
    create_debug_collision_geometry();
}

// 生成地形高度（使用简化的噪声函数）
float generate_terrain_height(int grid_x, int grid_z)
{
    // 使用简单的噪声函数生成地形高度
    float noise1 = sin(grid_x * 0.1f) * cos(grid_z * 0.1f);
    float noise2 = sin(grid_x * 0.05f) * cos(grid_z * 0.05f) * 0.5f;
    float noise3 = sin(grid_x * 0.02f) * cos(grid_z * 0.02f) * 0.25f;
    
    float height = (noise1 + noise2 + noise3) * 2.0f + 3.0f; // 基础高度3.0，变化范围约±3.0
    
    // 量化为整数高度
    return floor(height);
}

// 更新角色周围的地形
void update_terrain_around_character()
{
    if (!g_character)
        return;
    
    // 获取角色当前位置
    RVec3 char_pos = g_character->GetPosition();
    auto current_grid_pos = world_to_grid((float)char_pos.GetX(), (float)char_pos.GetZ());
    
    // 计算当前地形中心
    auto current_terrain_center = current_grid_pos;
    
    // 检查是否需要更新地形
    if (g_last_terrain_center != std::pair<int, int>{-999, -999})
    {
        int dx = abs(current_terrain_center.first - g_last_terrain_center.first);
        int dz = abs(current_terrain_center.second - g_last_terrain_center.second);
        
        // 如果角色没有移动足够远，不需要更新
        if (dx < g_terrain_update_threshold && dz < g_terrain_update_threshold)
            return;
    }
    
    cout << "更新地形，角色位置: (" << current_grid_pos.first << ", " << current_grid_pos.second << ")" << endl;
    
    // 计算新的100x100区域
    std::set<std::pair<int, int>> new_terrain_region;
    for (int dx = -g_terrain_radius; dx <= g_terrain_radius; ++dx)
    {
        for (int dz = -g_terrain_radius; dz <= g_terrain_radius; ++dz)
        {
            int grid_x = current_terrain_center.first + dx;
            int grid_z = current_terrain_center.second + dz;
            new_terrain_region.insert({grid_x, grid_z});
        }
    }
    
    // 移除过远的地形数据
    auto it = g_generated_terrain.begin();
    while (it != g_generated_terrain.end())
    {
        if (new_terrain_region.find(it->first) == new_terrain_region.end())
        {
            it = g_generated_terrain.erase(it);
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
        if (g_generated_terrain.find(grid_pos) == g_generated_terrain.end())
        {
            float height = generate_terrain_height(grid_pos.first, grid_pos.second);
            g_generated_terrain[grid_pos] = height;
            new_terrain_count++;
        }
    }
    
    g_last_terrain_center = current_terrain_center;
    
    cout << "地形更新完成，新增地形块: " << new_terrain_count << "，总地形块: " << g_generated_terrain.size() << endl;
    
    // 地形更新后，更新地面几何体
    update_ground_geometry();
}

// 更新地面几何体（基于动态生成的地形数据）
void update_ground_geometry()
{
    cout << "更新地面几何体（基于动态地形）..." << endl;
    
    // 删除旧的几何体
    if (g_ground_vao != 0)
    {
        glDeleteVertexArrays(1, &g_ground_vao);
        glDeleteBuffers(1, &g_ground_vbo);
        g_ground_vao = 0;
        g_ground_vbo = 0;
        g_ground_vertex_count = 0;
    }
    
    // 计算需要渲染的地形区域（基于生成的地形数据）
    if (g_generated_terrain.empty())
    {
        cout << "没有生成的地形数据，跳过地面几何体创建" << endl;
        return;
    }
    
    // 找到地形数据的边界
    int min_x = INT_MAX, max_x = INT_MIN;
    int min_z = INT_MAX, max_z = INT_MIN;
    
    for (const auto& terrain_pair : g_generated_terrain)
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
            auto terrain_it = g_generated_terrain.find({x, z});
            if (terrain_it == g_generated_terrain.end())
                continue;
                
            float height = terrain_it->second;
            
            // 每个块至少有顶面（6个顶点）
            estimated_vertices += 6;
            
            // 检查侧面是否需要渲染
            auto left_it = g_generated_terrain.find({x - 1, z});
            if (left_it == g_generated_terrain.end() || left_it->second < height)
                estimated_vertices += 6; // 左面
                
            auto right_it = g_generated_terrain.find({x + 1, z});
            if (right_it == g_generated_terrain.end() || right_it->second < height)
                estimated_vertices += 6; // 右面
                
            auto front_it = g_generated_terrain.find({x, z - 1});
            if (front_it == g_generated_terrain.end() || front_it->second < height)
                estimated_vertices += 6; // 前面
                
            auto back_it = g_generated_terrain.find({x, z + 1});
            if (back_it == g_generated_terrain.end() || back_it->second < height)
                estimated_vertices += 6; // 后面
        }
    }
    
    // 预分配内存
    std::vector<float> vertices;
    vertices.reserve(estimated_vertices * 8); // 每个顶点8个float
    
    cout << "预分配内存: " << estimated_vertices << " 个顶点" << endl;
    
    // 超高效的添加四边形函数
    auto add_quad_ultra_fast =
        [&](float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4, float nx, float ny, float nz)
        {
            vertices.insert(
                vertices.end(),
                {
                    x1, y1, z1, nx, ny, nz, 0.0f, 0.0f, // 顶点1
                    x2, y2, z2, nx, ny, nz, 1.0f, 0.0f, // 顶点2
                    x3, y3, z3, nx, ny, nz, 1.0f, 1.0f, // 顶点3
                    x1, y1, z1, nx, ny, nz, 0.0f, 0.0f, // 顶点4
                    x3, y3, z3, nx, ny, nz, 1.0f, 1.0f, // 顶点5
                    x4, y4, z4, nx, ny, nz, 0.0f, 1.0f  // 顶点6
                }
            );
        };
    
    int processed_tiles = 0;
    int total_tiles = (max_x - min_x + 1) * (max_z - min_z + 1);
    
    // 生成顶点数据
    for (int z = min_z; z <= max_z; ++z)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            auto terrain_it = g_generated_terrain.find({x, z});
            if (terrain_it == g_generated_terrain.end())
                continue;
                
            float height = terrain_it->second;
            
            // 计算世界坐标
            float world_x1 = (x - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            float world_x2 = (x + 1 - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            float world_z1 = (z - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            float world_z2 = (z + 1 - g_terrain_grid_size / 2.0f) * g_terrain_tile_size;
            
            float height_top = height;
            float height_bottom = height - 1.0f;
            
            // 顶面（总是渲染）
            add_quad_ultra_fast(
                world_x1, height_top, world_z1,
                world_x2, height_top, world_z1,
                world_x2, height_top, world_z2,
                world_x1, height_top, world_z2,
                0.0f, 1.0f, 0.0f
            );
            
            // 左面 (-X) - 只在高度变化时渲染
            auto left_it = g_generated_terrain.find({x - 1, z});
            if (left_it == g_generated_terrain.end() || left_it->second < height_top)
            {
                add_quad_ultra_fast(
                    world_x1, height_bottom, world_z1,
                    world_x1, height_bottom, world_z2,
                    world_x1, height_top, world_z2,
                    world_x1, height_top, world_z1,
                    -1.0f, 0.0f, 0.0f
                );
            }
            
            // 右面 (+X) - 只在高度变化时渲染
            auto right_it = g_generated_terrain.find({x + 1, z});
            if (right_it == g_generated_terrain.end() || right_it->second < height_top)
            {
                add_quad_ultra_fast(
                    world_x2, height_bottom, world_z1,
                    world_x2, height_bottom, world_z2,
                    world_x2, height_top, world_z2,
                    world_x2, height_top, world_z1,
                    1.0f, 0.0f, 0.0f
                );
            }
            
            // 前面 (-Z) - 只在高度变化时渲染
            auto front_it = g_generated_terrain.find({x, z - 1});
            if (front_it == g_generated_terrain.end() || front_it->second < height_top)
            {
                add_quad_ultra_fast(
                    world_x1, height_bottom, world_z1,
                    world_x2, height_bottom, world_z1,
                    world_x2, height_top, world_z1,
                    world_x1, height_top, world_z1,
                    0.0f, 0.0f, -1.0f
                );
            }
            
            // 后面 (+Z) - 只在高度变化时渲染
            auto back_it = g_generated_terrain.find({x, z + 1});
            if (back_it == g_generated_terrain.end() || back_it->second < height_top)
            {
                add_quad_ultra_fast(
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
    glGenVertexArrays(1, &g_ground_vao);
    glGenBuffers(1, &g_ground_vbo);
    
    glBindVertexArray(g_ground_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_ground_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // 设置顶点属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    g_ground_vertex_count = vertices.size() / 8; // 8 floats per vertex
    
    cout << "动态地面几何体创建完成: " << g_ground_vertex_count << " 个顶点" << endl;
    cout << "内存使用: " << (vertices.size() * sizeof(float) / 1024.0f / 1024.0f) << " MB" << endl;
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
    g_temp_allocator = new TempAllocatorImpl(160 * 1024 * 1024);

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

    // 初始化动态地面物理系统
    cout << "初始化动态地面物理系统..." << endl;
    g_active_physics_bodies.clear();
    g_last_character_grid_pos = {-999, -999}; // 重置为无效位置，强制初始更新
    
    // 初始化动态地形系统
    cout << "初始化动态地形系统..." << endl;
    g_generated_terrain.clear();
    g_last_terrain_center = {-999, -999}; // 重置为无效位置，强制初始更新

    // 创建角色控制器
    Ref<CharacterSettings> character_settings = new CharacterSettings();
    character_settings->mMaxSlopeAngle        = DegreesToRadians(45.0f);
    character_settings->mLayer                = Layers::MOVING;
    character_settings->mShape                = new CapsuleShape(0.4f, 0.3f); // 半高度0.4m，半径0.3m（总高度1.4m）
    character_settings->mFriction             = 0.5f;
    character_settings->mSupportingVolume     = Plane(Vec3::sAxisY(), -0.4f);

    g_character = new Character(character_settings, RVec3(0, 8, 0), Quat::sIdentity(), 0, g_physics_system);
    g_character->AddToPhysicsSystem(EActivation::Activate);

    cout << "Jolt Physics 初始化成功！" << endl;
    
    // 在物理系统初始化完成后创建蓝色实心几何体
    create_ground_collision_solid_geometry();
    
    return true;
}

// 更新物理
void update_physics(float delta_time)
{
    if (!g_physics_system || !g_character)
        return;

    // 计算摄像机角度（供移动和摄像机位置计算使用）
    float yaw_rad   = glm::radians(g_camera_yaw);
    float pitch_rad = glm::radians(g_camera_pitch);

    // 处理角色输入
    const float move_speed = 5.0f;

    // 计算摄像机的前方和右方向（基于 yaw 角，忽略 pitch）
    // 摄像机看向目标的方向 = -(offset的水平分量)
    Vec3 camera_forward(cosf(yaw_rad), 0, sinf(yaw_rad)); // 摄像机前方
    Vec3 camera_right(-sinf(yaw_rad), 0, cosf(yaw_rad));  // 摄像机右方 (up × forward)

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
        desired_velocity.SetY(4.0f); // 跳跃速度（可跳约1.5m高）
    }

    g_character->SetLinearVelocity(desired_velocity);

    // 更新地形系统
    update_terrain_around_character();
    
    // 更新动态地面物理系统
    update_dynamic_ground_physics();
    
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

    // 确保OpenGL状态正确初始化
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // 使用着色器程序
    glUseProgram(g_shader_program);

    // 设置矩阵
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
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

    // 绘制地形（使用视锥剔除）
    glm::mat4 ground_model = glm::mat4(1.0f);
    glm::mat4 view_proj    = projection * view;

    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(ground_model));
    glUniform3f(object_color_loc, 1.0f, 1.0f, 1.0f);
    glUniform1i(use_texture_loc, 1);
    glUniform1i(use_alpha_loc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_ground_texture);
    glUniform1i(glGetUniformLocation(g_shader_program, "textureSampler"), 0);

    glBindVertexArray(g_ground_vao);

    // 优化的渲染：直接绘制所有顶点（已经过视锥剔除优化）
    // 由于我们只生成了可见面的顶点，可以直接绘制整个缓冲区
    glDrawArrays(GL_TRIANGLES, 0, g_ground_vertex_count);

    // 可选：在控制台显示渲染统计
    static int frame_count = 0;
    if (++frame_count % 60 == 0) // 每60帧显示一次
    {
        cout << "渲染统计: 优化后顶点数 = " << g_ground_vertex_count << endl;
    }

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

    // 渲染调试碰撞网格（绿色线框）
    if (g_debug_collision_mesh)
    {
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniform3f(object_color_loc, 0.0f, 1.0f, 0.0f); // 绿色
        glUniform1i(use_texture_loc, 0);
        glUniform1i(use_alpha_loc, 0);

        glBindVertexArray(g_debug_vao);
        glDrawArrays(GL_LINES, 0, g_debug_vertex_count);
    }

    // 渲染地面碰撞体实心盒子（蓝色实心）
    if (g_debug_ground_collision_solid)
    {
        // 保存当前渲染状态
        GLboolean cull_face_enabled = glIsEnabled(GL_CULL_FACE);
        GLboolean polygon_offset_enabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
        
        // 禁用面剔除，确保所有面都可见
        // glDisable(GL_CULL_FACE);
        
        // 稍微偏移深度，确保蓝色盒子在地形之上显示
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glUniform3f(object_color_loc, 0.0f, 0.0f, 1.0f); // 蓝色
        glUniform1i(use_texture_loc, 0);
        glUniform1i(use_alpha_loc, 0);

        glBindVertexArray(g_ground_collision_vao);
        glDrawArrays(GL_TRIANGLES, 0, g_ground_collision_vertex_count);
        
        // 调试信息：每60帧显示一次
        static int debug_counter = 0;
        if (++debug_counter % 60 == 0)
        {
            cout << "渲染蓝色实心盒子: " << g_ground_collision_vertex_count << " 个顶点" << endl;
            cout << "渲染状态 - 面剔除: " << (glIsEnabled(GL_CULL_FACE) ? "开启" : "关闭") 
                 << ", 深度偏移: " << (glIsEnabled(GL_POLYGON_OFFSET_FILL) ? "开启" : "关闭") << endl;
        }
        
        // 完全恢复之前的渲染状态
        if (polygon_offset_enabled)
            glEnable(GL_POLYGON_OFFSET_FILL);
        else
            glDisable(GL_POLYGON_OFFSET_FILL);
            
        // if (cull_face_enabled)
        //     glEnable(GL_CULL_FACE);
        // else
        //     glDisable(GL_CULL_FACE);
    }

    // 渲染角色碰撞体调试网格（红色线框）
    if (g_debug_character_collision && g_character)
    {
        RVec3 char_pos = g_character->GetPosition();
        // 角色位置是胶囊体的中心，调试几何体也是以中心为原点创建的
        glm::mat4 character_debug_model = glm::translate(glm::mat4(1.0f), glm::vec3((float)char_pos.GetX(), (float)char_pos.GetY(), (float)char_pos.GetZ()));
        
        // 调试信息：显示角色位置
        static int debug_counter = 0;
        if (++debug_counter % 60 == 0) // 每秒显示一次
        {
            cout << "角色位置: (" << char_pos.GetX() << ", " << char_pos.GetY() << ", " << char_pos.GetZ() << ")" << endl;
        }
        
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(character_debug_model));
        glUniform3f(object_color_loc, 1.0f, 0.0f, 0.0f); // 红色
        glUniform1i(use_texture_loc, 0);
        glUniform1i(use_alpha_loc, 0);
        
        glBindVertexArray(g_character_debug_vao);
        glDrawArrays(GL_LINES, 0, g_character_debug_vertex_count);
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
    cout << "  O          - 切换地形碰撞网格显示（绿色线框）" << endl;
    cout << "  B          - 切换地面碰撞体显示（蓝色实心）" << endl;
    cout << "  C          - 切换角色碰撞体显示（红色线框）" << endl;
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
    // create_ground_geometry(); // 现在使用动态地形，不需要静态地形
    create_capsule_geometry(0.3f, 0.4f); // 半径0.3m，半高度0.4m（总高度1.4m）
    create_debug_collision_geometry(); // 创建调试碰撞网格
    create_character_debug_geometry(); // 创建角色碰撞体调试网格

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
