#include "jolt_character_test_app.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

JoltCharacterTestApp::JoltCharacterTestApp()
    : m_window(nullptr)
    , m_physics_manager(nullptr)
    , m_terrain_system(nullptr)
    , m_character_controller(nullptr)
    , m_camera(nullptr)
    , m_input_manager(nullptr)
    , m_shader_manager(nullptr)
    , m_renderer(nullptr)
    , m_initialized(false)
    , m_imgui_initialized(false)
    , m_last_time(0.0)
    , m_window_width(800)
    , m_window_height(600)
    , m_framebuffer_width(800)
    , m_framebuffer_height(600)
{
}

JoltCharacterTestApp::~JoltCharacterTestApp()
{
    // Shutdown();
}

bool JoltCharacterTestApp::Initialize()
{

    if (!InitializeGLFW())
    {
        cerr << "GLFW 初始化失败" << endl;
        return false;
    }

    if (!InitializeOpenGL())
    {
        cerr << "OpenGL 初始化失败" << endl;
        return false;
    }

    if (!InitializeSystems())
    {
        cerr << "系统初始化失败" << endl;
        return false;
    }

    if (!InitializeImGui())
    {
        cerr << "ImGui 初始化失败" << endl;
        return false;
    }

    m_initialized = true;
    return true;
}

void JoltCharacterTestApp::Run()
{
    if (!m_initialized)
    {
        cerr << "应用程序未初始化" << endl;
        return;
    }

    m_last_time = glfwGetTime();

    while (!glfwWindowShouldClose(m_window))
    {
        double current_time = glfwGetTime();
        float delta_time = (float)(current_time - m_last_time);
        m_last_time = current_time;

        // 限制时间步长
        if (delta_time > 0.1f)
            delta_time = 0.1f;

        glfwPollEvents();
        Update(delta_time);
        Render();
    }
}

void JoltCharacterTestApp::Shutdown()
{
    Cleanup();
    m_initialized = false;
    cout << "应用程序已关闭" << endl;
}

bool JoltCharacterTestApp::InitializeGLFW()
{
    // 初始化 GLFW
    if (!glfwInit())
    {
        cerr << "Failed to initialize GLFW" << endl;
        return false;
    }

    // 设置 OpenGL 版本
#ifdef __APPLE__
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
    m_window = glfwCreateWindow(m_window_width, m_window_height, "Jolt Character Controller Example", nullptr, nullptr);
    if (!m_window)
    {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    
    // 设置用户指针以便在回调中访问应用程序实例
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowSizeCallback(m_window, WindowSizeCallback);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
    glfwSwapInterval(1); // 启用垂直同步
    
    // 获取初始framebuffer大小
    glfwGetFramebufferSize(m_window, &m_framebuffer_width, &m_framebuffer_height);

    return true;
}

bool JoltCharacterTestApp::InitializeOpenGL()
{
    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Failed to initialize GLAD" << endl;
        return false;
    }

    cout << "OpenGL 版本: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL 版本: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    
    // 设置初始视口（使用framebuffer大小）
    glViewport(0, 0, m_framebuffer_width, m_framebuffer_height);

    // 设置 OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // 天蓝色背景

    return true;
}

bool JoltCharacterTestApp::InitializeImGui()
{
    // 设置 ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 启用键盘控制
    
    // 设置显示大小（使用framebuffer大小）
    io.DisplaySize = ImVec2((float)m_framebuffer_width, (float)m_framebuffer_height);

    // 设置 ImGui 样式
    ImGui::StyleColorsDark();

    // 设置平台/渲染器绑定
    const char* glsl_version = "#version 330";
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true))
    {
        cerr << "ImGui GLFW 初始化失败" << endl;
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        cerr << "ImGui OpenGL3 初始化失败" << endl;
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    // 设置中文字体
    // 尝试加载中文字体，如果失败则使用默认字体
    ImFont* font = nullptr;
    
    // 尝试多个可能的字体路径
    const char* font_paths[] = {
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc", 
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Arial.ttf",
        "/Library/Fonts/Arial.ttf"
    };
    
    for (const char* font_path : font_paths)
    {
        // 检查文件是否存在
        FILE* file = fopen(font_path, "rb");
        if (file != nullptr)
        {
            fclose(file);
            cout << "尝试加载字体: " << font_path << endl;
            font = io.Fonts->AddFontFromFileTTF(font_path, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
            if (font != nullptr)
            {
                cout << "字体加载成功: " << font_path << endl;
                break;
            }
            else
            {
                cout << "字体加载失败: " << font_path << endl;
            }
        }
    }
    
    // 如果所有字体文件都加载失败，使用默认字体但添加中文字符范围
    if (font == nullptr)
    {
        cout << "所有字体文件加载失败，使用默认字体" << endl;
        ImFontConfig config;
        config.GlyphRanges = io.Fonts->GetGlyphRangesChineseFull();
        font = io.Fonts->AddFontDefault(&config);
    }
    
    if (font != nullptr)
    {
        io.FontDefault = font;
        cout << "字体设置完成" << endl;
    }
    else
    {
        cout << "警告：无法设置任何字体" << endl;
    }

    // 构建字体纹理
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    m_imgui_initialized = true;
    cout << "ImGui 初始化成功" << endl;
    return true;
}

bool JoltCharacterTestApp::InitializeSystems()
{
    // 创建系统组件
    m_physics_manager = new PhysicsManager();
    m_terrain_system = new TerrainSystem();
    m_character_controller = new CharacterController();
    m_camera = new Camera();
    m_input_manager = new InputManager();
    m_shader_manager = new ShaderManager();
    m_renderer = new Renderer();

    // 先初始化地形系统
    if (!m_terrain_system->Initialize())
    {
        cerr << "地形系统初始化失败" << endl;
        return false;
    }

    // 使用地形系统初始化物理管理器
    if (!m_physics_manager->Initialize(m_terrain_system))
    {
        cerr << "物理管理器初始化失败" << endl;
        return false;
    }

    if (!m_character_controller->Initialize(m_physics_manager))
    {
        cerr << "角色控制器初始化失败" << endl;
        return false;
    }

    m_camera->Initialize();

    m_input_manager->Initialize(m_window);
    m_input_manager->SetCamera(m_camera);
    m_input_manager->SetTerrainSystem(m_terrain_system);
    m_input_manager->SetPhysicsManager(m_physics_manager);

    if (!m_shader_manager->Initialize())
    {
        cerr << "着色器管理器初始化失败" << endl;
        return false;
    }

    if (!m_renderer->Initialize())
    {
        cerr << "渲染器初始化失败" << endl;
        return false;
    }

    return true;
}

void JoltCharacterTestApp::Update(float delta_time)
{
    // 更新输入
    m_input_manager->Update();

    // 获取角色位置并更新地形
    RVec3 char_pos = m_character_controller->GetPosition();
    auto character_grid_pos = m_physics_manager->WorldToGrid((float)char_pos.GetX(), (float)char_pos.GetZ());
    m_terrain_system->Update(character_grid_pos);

    // 更新物理系统
    m_physics_manager->Update(delta_time);

    // 更新动态地面物理
    m_physics_manager->UpdateDynamicGroundPhysics(m_terrain_system->GetGeneratedTerrain(), character_grid_pos);

    // 更新角色控制器
    const InputState& input_state = m_input_manager->GetInputState();
    
    // 计算摄像机方向
    float yaw_rad = glm::radians(m_camera->GetYaw());
    glm::vec3 camera_forward(cosf(yaw_rad), 0, sinf(yaw_rad));
    glm::vec3 camera_right(-sinf(yaw_rad), 0, cosf(yaw_rad));
    
    m_character_controller->Update(delta_time, input_state, camera_forward, camera_right);

    // 更新摄像机
    m_camera->Update(char_pos);

    // 更新调试几何体
    const DebugOptions& debug_options = m_input_manager->GetDebugOptions();
    if (debug_options.debug_collision_mesh)
    {
        m_renderer->UpdateDebugCollisionGeometry(
            m_physics_manager->GetActivePhysicsBodies(),
            m_terrain_system->GetGeneratedTerrain()
        );
    }
    
    if (debug_options.debug_ground_collision_solid)
    {
        m_renderer->UpdateGroundCollisionSolidGeometry(
            m_physics_manager->GetActivePhysicsBodies(),
            m_terrain_system->GetGeneratedTerrain()
        );
    }
    
    if (debug_options.debug_side_wall_collision_solid)
    {
        m_renderer->UpdateSideWallCollisionGeometry(
            m_physics_manager->GetActiveSideWallBodies(),
            m_terrain_system->GetGeneratedTerrain()
        );
    }
}

void JoltCharacterTestApp::Render()
{
    // 计算宽高比（使用framebuffer大小）
    float aspect_ratio = (float)m_framebuffer_width / (float)m_framebuffer_height;
    
    m_renderer->Render(
        *m_camera,
        *m_terrain_system,
        *m_character_controller,
        *m_physics_manager,
        m_input_manager->GetDebugOptions(),
        aspect_ratio
    );
    
    // 渲染 ImGui
    double current_time = glfwGetTime();
    float delta_time = (float)(current_time - m_last_time);
    RenderImGui(delta_time);
    
    glfwSwapBuffers(m_window);
}

void JoltCharacterTestApp::RenderImGui(float delta_time)
{
    if (!m_imgui_initialized)
        return;
        
    // 开始新的 ImGui 帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 创建背景信息窗口
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("背景信息", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        // 性能信息
        ImGui::SeparatorText("性能");
        float fps = (delta_time > 0.0f) ? (1.0f / delta_time) : 0.0f;
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("帧时间: %.3f ms", delta_time * 1000.0f);
        
        // 窗口信息
        ImGui::SeparatorText("窗口");
        ImGui::Text("窗口大小: %dx%d", m_window_width, m_window_height);
        ImGui::Text("Framebuffer: %dx%d", m_framebuffer_width, m_framebuffer_height);
        float aspect_ratio = (float)m_framebuffer_width / (float)m_framebuffer_height;
        ImGui::Text("宽高比: %.3f", aspect_ratio);
        float scale_factor = (float)m_framebuffer_width / (float)m_window_width;
        ImGui::Text("缩放因子: %.2f", scale_factor);
        
        // 角色信息
        ImGui::SeparatorText("角色");
        RVec3 char_pos = m_character_controller->GetPosition();
        ImGui::Text("位置: (%.2f, %.2f, %.2f)", 
                    (float)char_pos.GetX(), 
                    (float)char_pos.GetY(), 
                    (float)char_pos.GetZ());
        
        Vec3 char_vel = m_character_controller->GetVelocity();
        float speed = char_vel.Length();
        ImGui::Text("速度: %.2f m/s", speed);
        ImGui::Text("速度向量: (%.2f, %.2f, %.2f)", 
                    char_vel.GetX(), 
                    char_vel.GetY(), 
                    char_vel.GetZ());
        
        // 摄像机信息
        ImGui::SeparatorText("摄像机");
        ImGui::Text("距离: %.2f", m_camera->GetDistance());
        ImGui::Text("偏航角: %.1f°", m_camera->GetYaw());
        ImGui::Text("俯仰角: %.1f°", m_camera->GetPitch());
        
        glm::vec3 cam_pos = m_camera->GetPosition();
        ImGui::Text("位置: (%.2f, %.2f, %.2f)", 
                    cam_pos.x, cam_pos.y, cam_pos.z);
        
        // 地形信息
        ImGui::SeparatorText("地形");
        auto char_grid_pos = m_physics_manager->WorldToGrid(
            (float)char_pos.GetX(), 
            (float)char_pos.GetZ()
        );
        ImGui::Text("网格位置: (%d, %d)", char_grid_pos.first, char_grid_pos.second);
        
        const auto& generated = m_terrain_system->GetGeneratedTerrain();
        ImGui::Text("已生成块数: %lu", generated.size());
        
        // 物理信息
        ImGui::SeparatorText("物理");
        const auto& active_bodies = m_physics_manager->GetActivePhysicsBodies();
        ImGui::Text("活动物理体: %lu", active_bodies.size());
        
        // 调试选项
        ImGui::SeparatorText("调试选项");
        const DebugOptions& debug_options = m_input_manager->GetDebugOptions();
        ImGui::Text("碰撞网格 (O): %s", debug_options.debug_collision_mesh ? "开启" : "关闭");
        ImGui::Text("地面碰撞 (B): %s", debug_options.debug_ground_collision_solid ? "开启" : "关闭");
        ImGui::Text("角色碰撞 (C): %s", debug_options.debug_character_collision ? "开启" : "关闭");
        
        // 控制说明
        ImGui::SeparatorText("控制");
        ImGui::TextWrapped("WASD - 移动");
        ImGui::TextWrapped("空格 - 跳跃");
        ImGui::TextWrapped("P - 切换鼠标捕获");
        ImGui::TextWrapped("O/B/C - 调试显示");
        ImGui::TextWrapped("滚轮 - 摄像机距离");
        ImGui::TextWrapped("ESC - 退出");
    }
    ImGui::End();

    // 渲染 ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // 确保视口在 ImGui 渲染后仍然正确（使用framebuffer大小）
    glViewport(0, 0, m_framebuffer_width, m_framebuffer_height);
}

void JoltCharacterTestApp::Cleanup()
{
    // 清理 ImGui
    CleanupImGui();
    
    // 清理系统组件
    if (m_renderer)
    {
        delete m_renderer;
        m_renderer = nullptr;
    }

    if (m_shader_manager)
    {
        delete m_shader_manager;
        m_shader_manager = nullptr;
    }

    if (m_input_manager)
    {
        delete m_input_manager;
        m_input_manager = nullptr;
    }

    if (m_camera)
    {
        delete m_camera;
        m_camera = nullptr;
    }

    if (m_character_controller)
    {
        delete m_character_controller;
        m_character_controller = nullptr;
    }

    if (m_terrain_system)
    {
        delete m_terrain_system;
        m_terrain_system = nullptr;
    }

    if (m_physics_manager)
    {
        delete m_physics_manager;
        m_physics_manager = nullptr;
    }

    // 清理 GLFW
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void JoltCharacterTestApp::CleanupImGui()
{
    if (m_imgui_initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_imgui_initialized = false;
        cout << "ImGui 已清理" << endl;
    }
}

void JoltCharacterTestApp::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
    // 获取应用程序实例
    JoltCharacterTestApp* app = static_cast<JoltCharacterTestApp*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->OnWindowResize(width, height);
    }
}

void JoltCharacterTestApp::OnWindowResize(int width, int height)
{
    // 更新窗口尺寸
    m_window_width = width;
    m_window_height = height;
    
    cout << "窗口大小调整为: " << width << "x" << height << endl;
}

void JoltCharacterTestApp::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // 获取应用程序实例
    JoltCharacterTestApp* app = static_cast<JoltCharacterTestApp*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->OnFramebufferResize(width, height);
    }
}

void JoltCharacterTestApp::OnFramebufferResize(int width, int height)
{
    // 更新framebuffer尺寸
    m_framebuffer_width = width;
    m_framebuffer_height = height;
    
    // 更新视口（使用framebuffer大小）
    glViewport(0, 0, width, height);
    
    // 通知 ImGui framebuffer大小变化
    if (m_imgui_initialized)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
    }
    
    cout << "Framebuffer大小调整为: " << width << "x" << height << endl;
}
