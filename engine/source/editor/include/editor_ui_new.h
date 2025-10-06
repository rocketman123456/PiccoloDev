#pragma once

#include "ui_components/modular_editor_ui.h"

namespace Piccolo
{
    // 新的模块化编辑器UI类
    class EditorUINew : public Editor::ModularEditorUI
    {
    public:
        EditorUINew();
        ~EditorUINew() = default;

        // 重写初始化方法以设置特定的编辑器配置
        virtual void initialize(WindowUIInitInfo init_info) override;
        
        // 编辑器特定的方法
        void showEditorUI();
        void setUIColorStyle();
        
    private:
        void setupEditorSpecificComponents();
    };
}
