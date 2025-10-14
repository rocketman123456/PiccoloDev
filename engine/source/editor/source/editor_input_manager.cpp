#include "editor/include/editor_input_manager.h"

#include "editor/include/editor.h"
#include "editor/include/editor_global_context.h"
#include "editor/include/editor_scene_manager.h"

#include "runtime/engine.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

namespace Piccolo
{
    void EditorInputManager::initialize() 
    { 
        initializeKeyMappings();
        registerInput(); 
    }

    void EditorInputManager::initializeKeyMappings()
    {
        // 初始化按键到命令的映射表
        m_key_command_map[GLFW_KEY_A]      = EditorCommand::camera_left;
        m_key_command_map[GLFW_KEY_S]      = EditorCommand::camera_back;
        m_key_command_map[GLFW_KEY_W]      = EditorCommand::camera_forward;
        m_key_command_map[GLFW_KEY_D]      = EditorCommand::camera_right;
        m_key_command_map[GLFW_KEY_Q]      = EditorCommand::camera_up;
        m_key_command_map[GLFW_KEY_E]      = EditorCommand::camera_down;
        m_key_command_map[GLFW_KEY_T]      = EditorCommand::translation_mode;
        m_key_command_map[GLFW_KEY_R]      = EditorCommand::rotation_mode;
        m_key_command_map[GLFW_KEY_C]      = EditorCommand::scale_mode;
        m_key_command_map[GLFW_KEY_ESCAPE] = EditorCommand::exit;
        m_key_command_map[GLFW_KEY_DELETE] = EditorCommand::delete_object;
    }

    void EditorInputManager::tick(float /*delta_time*/) { processEditorCommand(); }

    void EditorInputManager::registerInput()
    {
        // 使用 lambda 表达式代替 std::bind，提高代码可读性
        g_editor_global_context.m_window_system->registerOnResetFunc(
            [this]() { onReset(); });
        
        g_editor_global_context.m_window_system->registerOnCursorPosFunc(
            [this](double xpos, double ypos) { onCursorPos(xpos, ypos); });
        
        g_editor_global_context.m_window_system->registerOnCursorEnterFunc(
            [this](int entered) { onCursorEnter(entered); });
        
        g_editor_global_context.m_window_system->registerOnScrollFunc(
            [this](double xoffset, double yoffset) { onScroll(xoffset, yoffset); });
        
        g_editor_global_context.m_window_system->registerOnMouseButtonFunc(
            [this](int key, int action, int /*mods*/) { onMouseButtonClicked(key, action); });
        
        g_editor_global_context.m_window_system->registerOnWindowCloseFunc(
            [this]() { onWindowClosed(); });
        
        g_editor_global_context.m_window_system->registerOnKeyFunc(
            [this](int key, int scancode, int action, int mods) { onKey(key, scancode, action, mods); });
    }

    void EditorInputManager::updateCursorOnAxis(Vector2 cursor_uv)
    {
        if (g_editor_global_context.m_scene_manager->getEditorCamera())
        {
            Vector2 window_size(m_engine_window_size.x, m_engine_window_size.y);
            m_cursor_on_axis = g_editor_global_context.m_scene_manager->updateCursorOnAxis(cursor_uv, window_size);
        }
    }

    void EditorInputManager::processEditorCommand()
    {
        processCameraMovement();
        processSpecialCommands();
    }

    void EditorInputManager::processCameraMovement()
    {
        std::shared_ptr<RenderCamera> editor_camera = g_editor_global_context.m_scene_manager->getEditorCamera();
        
        // 空指针安全检查
        if (!editor_camera)
        {
            return;
        }

        Vector3 camera_relative_pos(0, 0, 0);
        const Quaternion camera_rotate = editor_camera->rotation().inverse();

        // 处理前后左右移动
        if (m_editor_command & static_cast<unsigned int>(EditorCommand::camera_forward))
        {
            camera_relative_pos += camera_rotate * Vector3(0, m_camera_speed, 0);
        }
        if (m_editor_command & static_cast<unsigned int>(EditorCommand::camera_back))
        {
            camera_relative_pos += camera_rotate * Vector3(0, -m_camera_speed, 0);
        }
        if (m_editor_command & static_cast<unsigned int>(EditorCommand::camera_left))
        {
            camera_relative_pos += camera_rotate * Vector3(-m_camera_speed, 0, 0);
        }
        if (m_editor_command & static_cast<unsigned int>(EditorCommand::camera_right))
        {
            camera_relative_pos += camera_rotate * Vector3(m_camera_speed, 0, 0);
        }

        // 处理上下移动（世界坐标系）
        if (m_editor_command & static_cast<unsigned int>(EditorCommand::camera_up))
        {
            camera_relative_pos += Vector3(0, 0, m_camera_speed);
        }
        if (m_editor_command & static_cast<unsigned int>(EditorCommand::camera_down))
        {
            camera_relative_pos += Vector3(0, 0, -m_camera_speed);
        }

        // 只在有移动时才更新相机位置
        if (camera_relative_pos.length() > 0.0f)
        {
            editor_camera->move(camera_relative_pos);
        }
    }

    void EditorInputManager::processSpecialCommands()
    {
        // 处理删除对象命令
        if (m_editor_command & static_cast<unsigned int>(EditorCommand::delete_object))
        {
            g_editor_global_context.m_scene_manager->onDeleteSelectedGObject();
        }
    }

    void EditorInputManager::onKeyInEditorMode(int key, int /*scancode*/, int action, int /*mods*/)
    {
        // 查找键盘映射
        auto it = m_key_command_map.find(key);
        if (it == m_key_command_map.end())
        {
            return; // 未映射的按键，直接返回
        }

        const unsigned int command_flag = static_cast<unsigned int>(it->second);

        if (action == GLFW_PRESS)
        {
            // 按下按键时设置对应的命令标志
            m_editor_command |= command_flag;
        }
        else if (action == GLFW_RELEASE)
        {
            // 释放按键时清除对应的命令标志
            m_editor_command &= ~command_flag;
        }
    }

    void EditorInputManager::onKey(int key, int scancode, int action, int mods)
    {
        if (g_is_editor_mode)
        {
            onKeyInEditorMode(key, scancode, action, mods);
        }
    }

    void EditorInputManager::onReset()
    {
        // to do
    }

    void EditorInputManager::onCursorPos(double xpos, double ypos)
    {
        if (!g_is_editor_mode)
        {
            return;
        }

        // 检查鼠标位置是否有效
        if (m_mouse_x < 0.0f || m_mouse_y < 0.0f)
        {
            m_mouse_x = static_cast<float>(xpos);
            m_mouse_y = static_cast<float>(ypos);
            return;
        }

        auto* window_system = g_editor_global_context.m_window_system;
        GLFWwindow* window = window_system->getWindow();

        // 右键拖拽：旋转相机
        if (window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            
            auto editor_camera = g_editor_global_context.m_scene_manager->getEditorCamera();
            if (editor_camera)
            {
                const float angular_velocity = getCameraAngularVelocity();
                const Vector2 delta(ypos - m_mouse_y, xpos - m_mouse_x);
                editor_camera->rotate(delta * angular_velocity);
            }
        }
        // 左键拖拽：移动物体
        else if (window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            
            g_editor_global_context.m_scene_manager->moveEntity(
                xpos,
                ypos,
                m_mouse_x,
                m_mouse_y,
                m_engine_window_pos,
                m_engine_window_size,
                m_cursor_on_axis,
                g_editor_global_context.m_scene_manager->getSelectedObjectMatrix());
        }
        // 无按键：更新光标在坐标轴上的位置
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            if (isCursorInRect(m_engine_window_pos, m_engine_window_size))
            {
                updateCursorOnAxis(calculateCursorUV());
            }
        }

        // 更新鼠标位置
        m_mouse_x = static_cast<float>(xpos);
        m_mouse_y = static_cast<float>(ypos);
    }

    void EditorInputManager::onCursorEnter(int entered)
    {
        if (!entered) // lost focus
        {
            m_mouse_x = m_mouse_y = -1.0f;
        }
    }

    void EditorInputManager::onScroll(double /*xoffset*/, double yoffset)
    {
        if (!g_is_editor_mode || !isCursorInRect(m_engine_window_pos, m_engine_window_size))
        {
            return;
        }

        // 右键 + 滚轮：调整相机移动速度
        if (g_editor_global_context.m_window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
        {
            if (yoffset > 0)
            {
                m_camera_speed *= k_camera_speed_increase_factor;
            }
            else
            {
                m_camera_speed *= k_camera_speed_decrease_factor;
            }
        }
        // 滚轮：缩放相机
        else
        {
            auto editor_camera = g_editor_global_context.m_scene_manager->getEditorCamera();
            if (editor_camera)
            {
                editor_camera->zoom(static_cast<float>(yoffset) * k_zoom_sensitivity);
            }
        }
    }

    void EditorInputManager::onMouseButtonClicked(int key, int /*action*/)
    {
        // 仅在编辑器模式且未选中坐标轴时处理
        if (!g_is_editor_mode || m_cursor_on_axis != k_invalid_axis_index)
        {
            return;
        }

        // 检查当前关卡是否有效
        std::shared_ptr<Level> current_active_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        if (!current_active_level)
        {
            return;
        }

        // 仅处理引擎窗口内的左键点击
        if (key == GLFW_MOUSE_BUTTON_LEFT && isCursorInRect(m_engine_window_pos, m_engine_window_size))
        {
            const Vector2 picked_uv = calculateCursorUV();
            const size_t select_mesh_id = g_editor_global_context.m_scene_manager->getGuidOfPickedMesh(picked_uv);
            const size_t gobject_id = g_editor_global_context.m_render_system->getGObjectIDByMeshID(select_mesh_id);
            
            g_editor_global_context.m_scene_manager->onGObjectSelected(gobject_id);
        }
    }

    void EditorInputManager::onWindowClosed() 
    { 
        g_editor_global_context.m_engine_runtime->shutdownEngine(); 
    }

    bool EditorInputManager::isCursorInRect(Vector2 pos, Vector2 size) const
    {
        return m_mouse_x >= pos.x && m_mouse_x <= pos.x + size.x && 
               m_mouse_y >= pos.y && m_mouse_y <= pos.y + size.y;
    }

    Vector2 EditorInputManager::calculateCursorUV() const
    {
        return Vector2(
            (m_mouse_x - m_engine_window_pos.x) / m_engine_window_size.x,
            (m_mouse_y - m_engine_window_pos.y) / m_engine_window_size.y
        );
    }

    float EditorInputManager::getCameraAngularVelocity() const
    {
        // 180 度对应全屏移动
        return 180.0f / Math::max(m_engine_window_size.x, m_engine_window_size.y);
    }
} // namespace Piccolo