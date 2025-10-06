#include "editor/include/ui_components/modular_editor_ui.h"
#include "editor/include/editor_global_context.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"
#include "runtime/resource/config_manager/config_manager.h"
#include <chrono>
#include <imgui.h>
#include <imgui_internal.h>
#include <stb_image.h>
#include <GLFW/glfw3.h>

namespace Piccolo
{
    namespace Editor
    {
        ModularEditorUI::ModularEditorUI() { initializeUIComponents(); }

        void ModularEditorUI::initialize(WindowUIInitInfo init_info)
        {
            std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
            ASSERT(config_manager);

            // 创建ImGui上下文
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            // 设置UI内容缩放
            float x_scale, y_scale;
            glfwGetWindowContentScale(init_info.window_system->getWindow(), &x_scale, &y_scale);
            float content_scale = fmaxf(1.0f, fmaxf(x_scale, y_scale));
            // 设置内容缩放回调
            glfwSetWindowContentScaleCallback(init_info.window_system->getWindow(), [](GLFWwindow* window, float x_scale, float y_scale) {
                // 更新ImGui的缩放
                ImGui::GetIO().FontGlobalScale = fmaxf(x_scale, y_scale);
            });

            // 配置ImGui
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigDockingAlwaysTabBar         = true;
            io.ConfigWindowsMoveFromTitleBarOnly = true;

            // 加载字体
            io.Fonts->AddFontFromFileTTF(config_manager->getEditorFontPath().generic_string().data(), content_scale * 16, nullptr, nullptr);
            io.Fonts->Build();

            // 应用主题
            applyTheme();

            // 设置窗口图标
            GLFWimage window_icon[2];
            window_icon[0].pixels =
                stbi_load(config_manager->getEditorBigIconPath().generic_string().data(), &window_icon[0].width, &window_icon[0].height, 0, 4);
            window_icon[1].pixels =
                stbi_load(config_manager->getEditorSmallIconPath().generic_string().data(), &window_icon[1].width, &window_icon[1].height, 0, 4);
            glfwSetWindowIcon(init_info.window_system->getWindow(), 2, window_icon);
            stbi_image_free(window_icon[0].pixels);
            stbi_image_free(window_icon[1].pixels);
        }

        void ModularEditorUI::preRender()
        {
            // 注意：ImGui::NewFrame() 已经在 UIPass::draw() 中调用
            // 这里只需要渲染UI内容

            // 渲染主菜单栏
            renderMainMenuBar();

            // 渲染停靠空间
            renderDockingSpace();

            // 渲染各个窗口
            renderWorldObjectsWindow();
            renderFileContentWindow();
            renderGameWindow();
            renderComponentInspector();
            renderTransformEditor();

            // 渲染其他UI组件
            for (auto& [name, component] : m_ui_components)
            {
                if (component && component->isVisible())
                {
                    component->render();
                }
            }
        }

        void ModularEditorUI::setSelectedObject(std::shared_ptr<GObject> object)
        {
            m_selected_object = object;

            // 更新组件检查器
            if (m_component_inspector)
            {
                m_component_inspector->setSelectedObject(object);
            }

            // 更新变换编辑器
            if (m_transform_editor && object)
            {
                auto transform_component = object->tryGetComponent<TransformComponent>("TransformComponent");
                if (transform_component)
                {
                    m_transform_editor->setTransformComponent(std::shared_ptr<TransformComponent>(transform_component, [](TransformComponent*){}));
                }
                else
                {
                    m_transform_editor->clearSelection();
                }
            }
        }

        void ModularEditorUI::clearSelection()
        {
            m_selected_object = nullptr;

            if (m_component_inspector)
                m_component_inspector->clearSelection();

            if (m_transform_editor)
                m_transform_editor->clearSelection();
        }

        void ModularEditorUI::registerUIComponent(const std::string& name, std::unique_ptr<UIComponentBase> component)
        {
            m_ui_components[name] = std::move(component);
        }

        UIComponentBase* ModularEditorUI::getUIComponent(const std::string& name)
        {
            auto it = m_ui_components.find(name);
            return (it != m_ui_components.end()) ? it->second.get() : nullptr;
        }

        void ModularEditorUI::removeUIComponent(const std::string& name) { m_ui_components.erase(name); }

        void ModularEditorUI::renderMainMenuBar()
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("文件"))
                {
                    if (ImGui::MenuItem("新建场景"))
                    {
                        // TODO: 实现新建场景
                    }
                    if (ImGui::MenuItem("打开场景"))
                    {
                        // TODO: 实现打开场景
                    }
                    if (ImGui::MenuItem("保存场景"))
                    {
                        // TODO: 实现保存场景
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("退出"))
                    {
                        // TODO: 实现退出
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("编辑"))
                {
                    if (ImGui::MenuItem("撤销"))
                    {
                        // TODO: 实现撤销
                    }
                    if (ImGui::MenuItem("重做"))
                    {
                        // TODO: 实现重做
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("视图"))
                {
                    ImGui::MenuItem("世界对象", nullptr, &m_asset_window_open);
                    ImGui::MenuItem("文件内容", nullptr, &m_file_content_window_open);
                    ImGui::MenuItem("游戏窗口", nullptr, &m_game_engine_window_open);
                    ImGui::MenuItem("组件检查器", nullptr, &m_component_inspector_open);
                    ImGui::MenuItem("变换编辑器", nullptr, &m_transform_editor_open);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("主题"))
                {
                    if (ImGui::MenuItem("深色主题"))
                    {
                        UITheme::getInstance().setTheme(UITheme::Theme::Dark);
                    }
                    if (ImGui::MenuItem("浅色主题"))
                    {
                        UITheme::getInstance().setTheme(UITheme::Theme::Light);
                    }
                    if (ImGui::MenuItem("自定义主题"))
                    {
                        UITheme::getInstance().setTheme(UITheme::Theme::Custom);
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        void ModularEditorUI::renderDockingSpace()
        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiConfigFlags_NoMouseCursorChange |
                                            ImGuiWindowFlags_NoBringToFrontOnFocus;

            const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(main_viewport->WorkPos, ImGuiCond_Always);
            std::array<int, 2> window_size = g_editor_global_context.m_window_system->getWindowSize();
            ImGui::SetNextWindowSize(ImVec2((float)window_size[0], (float)window_size[1]), ImGuiCond_Always);
            ImGui::SetNextWindowViewport(main_viewport->ID);

            ImGui::Begin("编辑器停靠空间", &m_editor_menu_window_open, window_flags);

            ImGuiID main_docking_id = ImGui::GetID("主停靠空间");
            if (ImGui::DockBuilderGetNode(main_docking_id) == nullptr)
            {
                setupDockingLayout();
            }

            // 注意：DockSpace不应该传递ImGuiDockNodeFlags_DockSpace标志
            ImGui::DockSpace(main_docking_id, ImVec2(0.0f, 0.0f), 0);
            ImGui::End();
        }

        void ModularEditorUI::renderWorldObjectsWindow()
        {
            if (!m_asset_window_open)
                return;

            ImGui::Begin("世界对象", &m_asset_window_open);

            // 渲染世界对象树
            auto current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
            if (current_level)
            {
                auto& objects = current_level->getAllGObjects();
                for (auto& object_pair : objects)
                {
                    auto object = object_pair.second;
                    if (ImGui::Selectable(object->getName().c_str(), m_selected_object == object))
                    {
                        setSelectedObject(object);
                    }
                }
            }

            ImGui::End();
        }

        void ModularEditorUI::renderFileContentWindow()
        {
            if (!m_file_content_window_open)
                return;

            ImGui::Begin("文件内容", &m_file_content_window_open);

            static ImGuiTableFlags flags =
                ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

            if (ImGui::BeginTable("文件内容", 2, flags))
            {
                ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                auto current_time = std::chrono::steady_clock::now();
                if (current_time - m_last_file_tree_update > std::chrono::seconds(1))
                {
                    m_editor_file_service.buildEngineFileTree();
                    m_last_file_tree_update = current_time;
                }

                EditorFileNode* editor_root_node = m_editor_file_service.getEditorRootNode();
                // TODO: 实现文件树渲染

                ImGui::EndTable();
            }

            ImGui::End();
        }

        void ModularEditorUI::renderGameWindow()
        {
            if (!m_game_engine_window_open)
                return;

            ImGui::Begin("游戏窗口", &m_game_engine_window_open);

            // 渲染游戏视图
            ImVec2 viewport_size = ImGui::GetContentRegionAvail();
            if (viewport_size.x > 0 && viewport_size.y > 0)
            {
                // TODO: 渲染游戏视图
                ImGui::Text("游戏视图 (%.0f x %.0f)", viewport_size.x, viewport_size.y);
            }

            ImGui::End();
        }

        void ModularEditorUI::renderComponentInspector()
        {
            if (!m_component_inspector_open || !m_component_inspector)
                return;

            m_component_inspector->render();
        }

        void ModularEditorUI::renderTransformEditor()
        {
            if (!m_transform_editor_open || !m_transform_editor)
                return;

            m_transform_editor->render();
        }

        void ModularEditorUI::setupDockingLayout()
        {
            ImGuiID main_docking_id = ImGui::GetID("主停靠空间");
            ImGui::DockBuilderRemoveNode(main_docking_id);
            ImGui::DockBuilderAddNode(main_docking_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(main_docking_id, ImGui::GetMainViewport()->WorkSize);

            // 创建停靠节点
            ImGuiID dock_id_left   = ImGui::DockBuilderSplitNode(main_docking_id, ImGuiDir_Left, 0.2f, nullptr, &main_docking_id);
            ImGuiID dock_id_right  = ImGui::DockBuilderSplitNode(main_docking_id, ImGuiDir_Right, 0.3f, nullptr, &main_docking_id);
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(main_docking_id, ImGuiDir_Down, 0.3f, nullptr, &main_docking_id);

            // 分配窗口到停靠节点
            ImGui::DockBuilderDockWindow("世界对象", dock_id_left);
            ImGui::DockBuilderDockWindow("文件内容", dock_id_bottom);
            ImGui::DockBuilderDockWindow("组件检查器", dock_id_right);
            ImGui::DockBuilderDockWindow("变换编辑器", dock_id_right);
            ImGui::DockBuilderDockWindow("游戏窗口", main_docking_id);

            ImGui::DockBuilderFinish(main_docking_id);
        }

        void ModularEditorUI::applyTheme() { UITheme::getInstance().setTheme(UITheme::Theme::Custom); }

        void ModularEditorUI::initializeUIComponents()
        {
            // 创建组件检查器
            m_component_inspector = std::make_unique<ComponentInspector>();

            // 创建变换编辑器
            m_transform_editor = std::make_unique<TransformComponentEditor>();

            // 注册所有UI组件
            UIComponentRegistrar::registerAllComponents(this);
        }

        // UI组件注册器实现
        void UIComponentRegistrar::registerAllComponents(ModularEditorUI* editor_ui)
        {
            // 这里可以注册更多的UI组件
            // 例如：材质编辑器、动画编辑器、物理属性编辑器等
        }
    } // namespace Editor
} // namespace Piccolo
