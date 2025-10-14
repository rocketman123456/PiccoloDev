#pragma once

#include "runtime/core/math/vector2.h"

#include <unordered_map>

namespace Piccolo
{
    class PiccoloEditor;

    // 编辑器命令枚举，使用位标志表示可同时执行的命令
    enum class EditorCommand : unsigned int
    {
        camera_left      = 1 << 0,  // A
        camera_back      = 1 << 1,  // S
        camera_forward   = 1 << 2,  // W (修正拼写)
        camera_right     = 1 << 3,  // D
        camera_up        = 1 << 4,  // Q
        camera_down      = 1 << 5,  // E
        translation_mode = 1 << 6,  // T
        rotation_mode    = 1 << 7,  // R
        scale_mode       = 1 << 8,  // C
        exit             = 1 << 9,  // Esc
        delete_object    = 1 << 10, // Delete
    };

    /**
     * @brief 编辑器输入管理器
     * 
     * 负责处理编辑器模式下的所有输入事件，包括：
     * - 键盘输入（相机移动、模式切换等）
     * - 鼠标输入（相机旋转、物体选择、拖拽等）
     * - 滚轮输入（相机缩放、速度调整）
     */
    class EditorInputManager
    {
    public:
        // 常量定义
        static constexpr size_t  k_invalid_axis_index  = 3;  // 无效的轴索引
        static constexpr float   k_default_camera_speed = 0.05f;
        static constexpr float   k_camera_speed_increase_factor = 1.2f;
        static constexpr float   k_camera_speed_decrease_factor = 0.8f;
        static constexpr float   k_zoom_sensitivity = 2.0f;
        static constexpr float   k_default_window_width = 1280.0f;
        static constexpr float   k_default_window_height = 768.0f;

        /**
         * @brief 初始化输入管理器
         */
        void initialize();

        /**
         * @brief 每帧更新
         * @param delta_time 帧间隔时间
         */
        void tick(float delta_time);

        /**
         * @brief 注册输入回调函数
         */
        void registerInput();

        /**
         * @brief 更新光标在坐标轴上的位置
         * @param cursor_uv 归一化的光标坐标 (0-1)
         */
        void updateCursorOnAxis(Vector2 cursor_uv);

        /**
         * @brief 处理编辑器命令
         */
        void processEditorCommand();

        /**
         * @brief 编辑器模式下的按键处理
         */
        void onKeyInEditorMode(int key, int scancode, int action, int mods);

        // 输入事件回调
        void onKey(int key, int scancode, int action, int mods);
        void onReset();
        void onCursorPos(double xpos, double ypos);
        void onCursorEnter(int entered);
        void onScroll(double xoffset, double yoffset);
        void onMouseButtonClicked(int key, int action);
        void onWindowClosed();

        /**
         * @brief 判断光标是否在指定矩形区域内
         */
        bool isCursorInRect(Vector2 pos, Vector2 size) const;

        // Getter 方法
        Vector2 getEngineWindowPos() const { return m_engine_window_pos; }
        Vector2 getEngineWindowSize() const { return m_engine_window_size; }
        float   getCameraSpeed() const { return m_camera_speed; }

        // Setter 方法
        void setEngineWindowPos(Vector2 new_window_pos) { m_engine_window_pos = new_window_pos; }
        void setEngineWindowSize(Vector2 new_window_size) { m_engine_window_size = new_window_size; }
        void resetEditorCommand() { m_editor_command = 0; }

    private:
        /**
         * @brief 初始化键盘映射表
         */
        void initializeKeyMappings();

        /**
         * @brief 处理相机移动命令
         */
        void processCameraMovement();

        /**
         * @brief 处理特殊命令（删除物体等）
         */
        void processSpecialCommands();

        /**
         * @brief 计算光标在引擎窗口中的UV坐标
         */
        Vector2 calculateCursorUV() const;

        /**
         * @brief 获取相机角速度
         */
        float getCameraAngularVelocity() const;

        // 窗口相关
        Vector2 m_engine_window_pos {0.0f, 0.0f};
        Vector2 m_engine_window_size {k_default_window_width, k_default_window_height};
        
        // 鼠标状态
        float m_mouse_x {0.0f};
        float m_mouse_y {0.0f};
        
        // 相机控制
        float m_camera_speed {k_default_camera_speed};

        // 坐标轴和命令状态
        size_t       m_cursor_on_axis {k_invalid_axis_index};
        unsigned int m_editor_command {0};

        // 键盘映射表：键码 -> 编辑器命令
        std::unordered_map<int, EditorCommand> m_key_command_map;
    };
} // namespace Piccolo
