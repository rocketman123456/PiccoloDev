#pragma once

#include "editor/include/axis.h"
#include "editor/include/ui_components/modular_editor_ui.h"

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

    // 新的模块化编辑器UI类
    class EditorUI : public Editor::ModularEditorUI
    {
    public:
        EditorUI();
        ~EditorUI() = default;

        // 重写初始化方法以设置特定的编辑器配置
        virtual void initialize(WindowUIInitInfo init_info) override;
        
        // 编辑器特定的方法
        void showEditorUI();
        void setUIColorStyle();
        
        // 对象选择管理
        void setSelectedObject(std::shared_ptr<GObject> object);
        void clearSelection();

    private:
        // 保留一些原有的功能作为兼容性方法
        void onFileContentItemClicked(EditorFileNode* node);
        void buildEditorFileAssetsUITree(EditorFileNode* node);
        void drawAxisToggleButton(const char* string_id, bool check_state, int axis_mode);
        void createClassUI(Reflection::ReflectionInstance& instance);
        void createLeafNodeUI(Reflection::ReflectionInstance& instance);
        std::string getLeafUINodeParentLabel();
        
        // 原有的UI窗口方法
        void showEditorMenu(bool* p_open);
        void showEditorWorldObjectsWindow(bool* p_open);
        void showEditorFileContentWindow(bool* p_open);
        void showEditorGameWindow(bool* p_open);
        void showEditorDetailWindow(bool* p_open);

        // 原有的UI创建器（用于兼容性）
        std::unordered_map<std::string, std::function<void(std::string, void*)>> m_editor_ui_creator;
        std::unordered_map<std::string, unsigned int> m_new_object_index_map;
        
        // 原有的窗口状态变量
        bool m_detail_window_open = true;
    };
} // namespace Piccolo
