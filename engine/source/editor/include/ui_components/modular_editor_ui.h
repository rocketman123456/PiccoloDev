#pragma once

#include "editor/include/editor_file_service.h"
#include "editor/include/ui_components/component_inspector.h"
#include "editor/include/ui_components/transform_component_editor.h"
#include "editor/include/ui_components/ui_component_base.h"
#include "editor/include/ui_components/ui_theme.h"

#include "runtime/function/framework/object/object.h"
#include "runtime/function/ui/window_ui.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Piccolo
{
    namespace Editor
    {
        // 模块化编辑器UI管理器
        class ModularEditorUI : public WindowUI
        {
        public:
            ModularEditorUI();
            ~ModularEditorUI() = default;

            // WindowUI接口
            virtual void initialize(WindowUIInitInfo init_info) override;
            virtual void preRender() override final;

            // 对象选择管理
            void setSelectedObject(std::shared_ptr<GObject> object);
            void clearSelection();

            // UI组件管理
            void             registerUIComponent(const std::string& name, std::unique_ptr<UIComponentBase> component);
            UIComponentBase* getUIComponent(const std::string& name);
            void             removeUIComponent(const std::string& name);

        protected:
            // 主要UI窗口
            void renderMainMenuBar();
            void renderDockingSpace();
            void renderWorldObjectsWindow();
            void renderFileContentWindow();
            void renderGameWindow();
            void renderComponentInspector();
            void renderTransformEditor();

            // 辅助方法
            void setupDockingLayout();
            void applyTheme();
            void initializeUIComponents();

            // UI组件
            std::unique_ptr<ComponentInspector>                               m_component_inspector;
            std::unique_ptr<TransformComponentEditor>                         m_transform_editor;
            std::unordered_map<std::string, std::unique_ptr<UIComponentBase>> m_ui_components;

            // 文件服务
            EditorFileService                                  m_editor_file_service;
            std::chrono::time_point<std::chrono::steady_clock> m_last_file_tree_update;

            // 窗口状态
            bool m_editor_menu_window_open  = true;
            bool m_asset_window_open        = true;
            bool m_game_engine_window_open  = true;
            bool m_file_content_window_open = true;
            bool m_component_inspector_open = true;
            bool m_transform_editor_open    = true;

            // 选中的对象
            std::shared_ptr<GObject> m_selected_object;
        };

        // UI组件注册器
        class UIComponentRegistrar
        {
        public:
            static void registerAllComponents(ModularEditorUI* editor_ui);
        };
    } // namespace Editor
} // namespace Piccolo
