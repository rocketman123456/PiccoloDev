#include "editor/include/editor_ui_new.h"
#include "editor/include/editor_global_context.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include <imgui.h>

namespace Piccolo
{
    EditorUINew::EditorUINew()
    {
        setupEditorSpecificComponents();
    }

    void EditorUINew::initialize(WindowUIInitInfo init_info)
    {
        // 调用基类初始化
        ModularEditorUI::initialize(init_info);
        
        // 设置编辑器特定的样式
        setUIColorStyle();
    }

    void EditorUINew::showEditorUI()
    {
        // 这个方法现在由基类的preRender()处理
        // 保留此方法以保持向后兼容性
    }

    void EditorUINew::setUIColorStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        
        // 编辑器特定的样式调整
        style.WindowPadding   = ImVec2(1.0, 0);
        style.FramePadding    = ImVec2(14.0, 2.0f);
        style.ChildBorderSize = 0.0f;
        style.FrameRounding   = 5.0f;
        style.FrameBorderSize = 1.5f;
        
        // 应用自定义主题
        Editor::UITheme::getInstance().setTheme(Editor::UITheme::Theme::Custom);
    }

    void EditorUINew::setupEditorSpecificComponents()
    {
        // 这里可以设置编辑器特定的UI组件
        // 例如：工具栏、状态栏、快捷键面板等
    }
}
