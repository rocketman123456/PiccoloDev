#pragma once

#include "editor/include/axis.h"

#include "runtime/core/math/vector2.h"

#include "runtime/function/framework/object/object.h"
#include "runtime/function/ui/window_ui.h"

#include "editor/include/editor_file_service.h"

#include <chrono>
#include <map>
#include <vector>

namespace Piccolo
{
    class PiccoloEditor;
    class WindowSystem;
    class RenderSystem;

    class EditorUI : public WindowUI
    {
    public:
        EditorUI();

    private:
        // UI Constants
        static constexpr float k_menu_bar_height = 18.0f;
        static constexpr float k_right_panel_width_ratio = 0.25f;
        static constexpr float k_bottom_panel_height_ratio = 0.30f;
        static constexpr float k_asset_panel_width_ratio = 0.30f;
        static constexpr float k_axis_button_area_width = 100.0f;
        static constexpr float k_font_size_scale = 16.0f;
        static constexpr int k_file_tree_update_interval_seconds = 1;
        static constexpr size_t k_name_buffer_size = 128;

        // File management
        void        onFileContentItemClicked(EditorFileNode* node);
        void        buildEditorFileAssetsUITree(EditorFileNode* node, const char* filter);
        static bool fileNodeMatchesFilter(EditorFileNode* node, const char* filter);
        
        // UI drawing helpers
        static void drawAxisToggleButton(const char* string_id, bool check_state, int axis_mode);
        static void drawVecControl(const std::string& label, Vector3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
        static void drawVecControl(const std::string& label, Quaternion& values, float resetValue = 0.0f, float columnWidth = 100.0f);
        
        // Reflection UI creators
        void        createClassUI(Reflection::ReflectionInstance& instance);
        void        createLeafNodeUI(Reflection::ReflectionInstance& instance);
        void        registerUICreators();
        std::string getLeafUINodeParentLabel();

        // Window display methods
        void showEditorUI();
        void showEditorMenu(bool* p_open);
        static void showEditorWorldObjectsWindow(bool* p_open);
        void showEditorFileContentWindow(bool* p_open);
        static void showEditorGameWindow(bool* p_open);
        void showEditorDetailWindow(bool* p_open);

        // Style setup
        static void setUIColorStyle();

    public:
        virtual void initialize(WindowUIInitInfo init_info) override final;
        virtual void preRender() override final;

    private:
        // UI creators
        std::unordered_map<std::string, std::function<void(std::string, void*)>> m_editor_ui_creator;
        std::unordered_map<std::string, unsigned int>                            m_new_object_index_map;
        
        // File service
        EditorFileService                                                        m_editor_file_service;
        std::chrono::time_point<std::chrono::steady_clock>                       m_last_file_tree_update;

        // Tree node state management (moved from global)
        std::vector<std::pair<std::string, bool>>                                m_editor_node_state_array;
        int                                                                      m_node_depth = -1;

        // Window states
        bool m_editor_menu_window_open       = true;
        bool m_asset_window_open             = true;
        bool m_game_engine_window_open       = true;
        bool m_file_content_window_open      = true;
        bool m_detail_window_open            = true;
        bool m_scene_lights_window_open      = true;
        bool m_scene_lights_data_window_open = true;

        // UI search buffers moved to function statics to avoid global state bloat
    };
} // namespace Piccolo
