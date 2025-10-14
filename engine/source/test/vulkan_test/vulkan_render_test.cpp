#include "asset_manager.h"
#include "renderer.h"
#include "scene_manager.h"
#include "input_manager.h"

#include <volk.h>

#define VK_NO_PROTOTYPES
#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

// const std::string MODEL_PATH   = "models/viking_room/viking_room.obj";
// const std::string TEXTURE_PATH = "models/viking_room/viking_room.png";
// const std::string MODEL_PATH   = "models/Fox/glTF/Fox.gltf";
// const std::string TEXTURE_PATH = "models/Fox/glTF/Texture.png";
const std::string MODEL_PATH   = "models/Cube/glTF/Cube.gltf";
const std::string TEXTURE_PATH = "models/Cube/glTF/Cube_BaseColor.png";

// Forward declaration for framebuffer resize callback
class VulkanApplication;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

class VulkanApplication
{
public:
    void run()
    {
        initWindow();
        initResources();
        mainLoop();
        cleanup();
    }

    GLFWwindow* getWindow() const { return m_window; }
    Renderer*   getRenderer() const { return m_renderer.get(); }

private:
    GLFWwindow*                   m_window = nullptr;
    std::unique_ptr<Renderer>     m_renderer;
    std::unique_ptr<AssetManager> m_assetManager;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<InputManager> m_inputManager;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Renderer - Optimized Architecture", nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

        m_startTime = std::chrono::high_resolution_clock::now();
    }

    void initResources()
    {
        // Create renderer
        m_renderer = std::make_unique<Renderer>(m_window, WIDTH, HEIGHT);
        m_renderer->initialize();

        // Create asset manager
        m_assetManager = std::make_unique<AssetManager>(
            m_renderer->getDevice(), m_renderer->getPhysicalDevice(), m_renderer->getCommandPool(), m_renderer->getGraphicsQueue()
        );

        // Load assets
        auto mesh    = m_assetManager->loadModel(MODEL_PATH);
        auto texture = m_assetManager->loadTexture(TEXTURE_PATH);

        // Create scene manager
        m_sceneManager = std::make_unique<SceneManager>();

        // Create input manager
        m_inputManager = std::make_unique<InputManager>(m_window);
        m_inputManager->setSceneManager(m_sceneManager.get());

        // Create a scene object
        auto* vikingRoom               = m_sceneManager->createObject("viking_room");
        vikingRoom->mesh               = mesh;
        vikingRoom->texture            = texture;
        vikingRoom->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);

        std::cout << "Resources initialized successfully!" << std::endl;
        std::cout << "Loaded model with " << mesh->vertices.size() << " vertices and " << mesh->indices.size() << " indices" << std::endl;
        std::cout << "Controls: Mouse to look around, WASD to move, Space/Shift for up/down, Scroll for zoom, ESC to exit" << std::endl;
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            drawFrame();
        }
    }

    void drawFrame()
    {
        // Calculate delta time
        auto  currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime   = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_startTime).count();
        m_startTime       = currentTime;

        // Update input
        m_inputManager->update(deltaTime);

        // Update scene
        m_sceneManager->update(deltaTime);

        // Render
        m_renderer->beginFrame();
        m_renderer->drawScene(*m_sceneManager, *m_assetManager);
        m_renderer->endFrame();
    }

    void cleanup()
    {
        m_inputManager.reset();
        m_sceneManager.reset();
        m_assetManager.reset();
        m_renderer.reset();

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
};

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    if (app && app->getRenderer())
    {
        app->getRenderer()->onFramebufferResize();
    }
}

int main()
{
    VulkanApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
