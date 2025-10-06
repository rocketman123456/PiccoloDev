#include "editor/include/ui_components/ui_theme.h"

#include <imgui.h>

namespace Piccolo
{
    namespace Editor
    {
        // 颜色常量定义
        const ImVec4 UITheme::Colors::Primary        = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
        const ImVec4 UITheme::Colors::PrimaryHovered = ImVec4(0.3f, 0.7f, 1.0f, 1.0f);
        const ImVec4 UITheme::Colors::PrimaryActive  = ImVec4(0.1f, 0.5f, 0.9f, 1.0f);

        const ImVec4 UITheme::Colors::Background        = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        const ImVec4 UITheme::Colors::BackgroundHovered = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        const ImVec4 UITheme::Colors::BackgroundActive  = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

        const ImVec4 UITheme::Colors::Text         = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        const ImVec4 UITheme::Colors::TextDisabled = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        const ImVec4 UITheme::Colors::TextSelected = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        const ImVec4 UITheme::Colors::Border       = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        const ImVec4 UITheme::Colors::BorderShadow = ImVec4(0.0f, 0.0f, 0.0f, 0.3f);

        const ImVec4 UITheme::Colors::Header        = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        const ImVec4 UITheme::Colors::HeaderHovered = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        const ImVec4 UITheme::Colors::HeaderActive  = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

        const ImVec4 UITheme::Colors::Button        = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        const ImVec4 UITheme::Colors::ButtonHovered = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        const ImVec4 UITheme::Colors::ButtonActive  = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

        const ImVec4 UITheme::Colors::FrameBg        = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        const ImVec4 UITheme::Colors::FrameBgHovered = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        const ImVec4 UITheme::Colors::FrameBgActive  = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

        const ImVec4 UITheme::Colors::AxisX = ImVec4(0.8f, 0.1f, 0.15f, 1.0f);
        const ImVec4 UITheme::Colors::AxisY = ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
        const ImVec4 UITheme::Colors::AxisZ = ImVec4(0.1f, 0.25f, 0.8f, 1.0f);
        const ImVec4 UITheme::Colors::AxisW = ImVec4(0.7f, 0.7f, 0.1f, 1.0f);

        // UI主题管理器实现
        UITheme& UITheme::getInstance()
        {
            static UITheme instance;
            return instance;
        }

        void UITheme::setTheme(Theme theme)
        {
            m_current_theme = theme;
            
            switch (theme)
            {
                case Theme::Dark:
                    ImGui::StyleColorsDark();
                    break;
                case Theme::Light:
                    ImGui::StyleColorsLight();
                    break;
                case Theme::Custom:
                    applyCustomTheme();
                    break;
            }
        }

        void UITheme::applyCustomTheme()
        {
            ImGuiStyle& style = ImGui::GetStyle();
            
            // 基础样式设置
            style.WindowPadding     = ImVec2(8.0f, 8.0f);
            style.FramePadding      = ImVec2(4.0f, 3.0f);
            style.CellPadding       = ImVec2(4.0f, 2.0f);
            style.ItemSpacing       = ImVec2(8.0f, 4.0f);
            style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
            style.IndentSpacing     = 21.0f;
            style.ScrollbarSize     = 14.0f;
            style.GrabMinSize       = 10.0f;
            
            // 圆角和边框
            style.WindowRounding    = 5.0f;
            style.ChildRounding     = 5.0f;
            style.FrameRounding     = 3.0f;
            style.PopupRounding     = 5.0f;
            style.ScrollbarRounding = 9.0f;
            style.GrabRounding      = 3.0f;
            style.TabRounding       = 4.0f;
            
            // 边框大小
            style.WindowBorderSize  = 1.0f;
            style.ChildBorderSize   = 1.0f;
            style.PopupBorderSize   = 1.0f;
            style.FrameBorderSize   = 0.0f;
            style.TabBorderSize     = 0.0f;
            
            setCustomColors();
        }

        void UITheme::setCustomColors()
        {
            ImGuiStyle& style = ImGui::GetStyle();
            
            // 应用自定义颜色
            style.Colors[ImGuiCol_Text]                  = Colors::Text;
            style.Colors[ImGuiCol_TextDisabled]          = Colors::TextDisabled;
            style.Colors[ImGuiCol_WindowBg]              = Colors::Background;
            style.Colors[ImGuiCol_ChildBg]               = Colors::Background;
            style.Colors[ImGuiCol_PopupBg]               = Colors::Background;
            style.Colors[ImGuiCol_Border]                = Colors::Border;
            style.Colors[ImGuiCol_BorderShadow]          = Colors::BorderShadow;
            style.Colors[ImGuiCol_FrameBg]               = Colors::FrameBg;
            style.Colors[ImGuiCol_FrameBgHovered]        = Colors::FrameBgHovered;
            style.Colors[ImGuiCol_FrameBgActive]         = Colors::FrameBgActive;
            style.Colors[ImGuiCol_TitleBg]               = Colors::Header;
            style.Colors[ImGuiCol_TitleBgActive]         = Colors::HeaderActive;
            style.Colors[ImGuiCol_TitleBgCollapsed]      = Colors::Header;
            style.Colors[ImGuiCol_MenuBarBg]             = Colors::Header;
            style.Colors[ImGuiCol_ScrollbarBg]           = Colors::Background;
            style.Colors[ImGuiCol_ScrollbarGrab]         = Colors::Button;
            style.Colors[ImGuiCol_ScrollbarGrabHovered]  = Colors::ButtonHovered;
            style.Colors[ImGuiCol_ScrollbarGrabActive]   = Colors::ButtonActive;
            style.Colors[ImGuiCol_CheckMark]             = Colors::Primary;
            style.Colors[ImGuiCol_SliderGrab]            = Colors::Primary;
            style.Colors[ImGuiCol_SliderGrabActive]      = Colors::PrimaryActive;
            style.Colors[ImGuiCol_Button]                = Colors::Button;
            style.Colors[ImGuiCol_ButtonHovered]         = Colors::ButtonHovered;
            style.Colors[ImGuiCol_ButtonActive]          = Colors::ButtonActive;
            style.Colors[ImGuiCol_Header]                = Colors::Header;
            style.Colors[ImGuiCol_HeaderHovered]         = Colors::HeaderHovered;
            style.Colors[ImGuiCol_HeaderActive]          = Colors::HeaderActive;
            style.Colors[ImGuiCol_Separator]             = Colors::Border;
            style.Colors[ImGuiCol_SeparatorHovered]      = Colors::Primary;
            style.Colors[ImGuiCol_SeparatorActive]       = Colors::PrimaryActive;
            style.Colors[ImGuiCol_ResizeGrip]            = Colors::Button;
            style.Colors[ImGuiCol_ResizeGripHovered]     = Colors::ButtonHovered;
            style.Colors[ImGuiCol_ResizeGripActive]      = Colors::ButtonActive;
            style.Colors[ImGuiCol_Tab]                   = Colors::Button;
            style.Colors[ImGuiCol_TabHovered]            = Colors::ButtonHovered;
            style.Colors[ImGuiCol_TabActive]             = Colors::ButtonActive;
            style.Colors[ImGuiCol_TabUnfocused]          = Colors::Button;
            style.Colors[ImGuiCol_TabUnfocusedActive]    = Colors::ButtonActive;
            style.Colors[ImGuiCol_DockingPreview]        = Colors::Primary;
            style.Colors[ImGuiCol_DockingEmptyBg]        = Colors::Background;
            style.Colors[ImGuiCol_PlotLines]             = Colors::Primary;
            style.Colors[ImGuiCol_PlotLinesHovered]      = Colors::PrimaryHovered;
            style.Colors[ImGuiCol_PlotHistogram]         = Colors::Primary;
            style.Colors[ImGuiCol_PlotHistogramHovered]  = Colors::PrimaryHovered;
            style.Colors[ImGuiCol_TableHeaderBg]         = Colors::Header;
            style.Colors[ImGuiCol_TableBorderStrong]     = Colors::Border;
            style.Colors[ImGuiCol_TableBorderLight]      = Colors::Border;
            style.Colors[ImGuiCol_TableRowBg]            = Colors::Background;
            style.Colors[ImGuiCol_TableRowBgAlt]         = Colors::BackgroundHovered;
            style.Colors[ImGuiCol_TextSelectedBg]        = Colors::Primary;
            style.Colors[ImGuiCol_DragDropTarget]        = Colors::Primary;
            style.Colors[ImGuiCol_NavHighlight]          = Colors::Primary;
            style.Colors[ImGuiCol_NavWindowingHighlight] = Colors::Primary;
            style.Colors[ImGuiCol_NavWindowingDimBg]     = Colors::Background;
            style.Colors[ImGuiCol_ModalWindowDimBg]      = Colors::Background;
        }

        // UI样式助手实现
        void UIStyleHelper::pushButtonStyle(const ImVec4& color, const ImVec4& hovered_color, const ImVec4& active_color)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered_color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, active_color);
        }

        void UIStyleHelper::popButtonStyle()
        {
            ImGui::PopStyleColor(3);
        }

        void UIStyleHelper::pushFrameStyle(float rounding, float border_size)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, border_size);
        }

        void UIStyleHelper::popFrameStyle()
        {
            ImGui::PopStyleVar(2);
        }

        void UIStyleHelper::pushTextStyle(const ImVec4& color)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
        }

        void UIStyleHelper::popTextStyle()
        {
            ImGui::PopStyleColor(1);
        }

        ImVec2 UIStyleHelper::calculateButtonSize(const char* text, float padding)
        {
            ImVec2 text_size = ImGui::CalcTextSize(text);
            return ImVec2(text_size.x + padding * 2, text_size.y + padding * 2);
        }

        void UIStyleHelper::centerText(const char* text)
        {
            float window_width = ImGui::GetWindowWidth();
            float text_width = ImGui::CalcTextSize(text).x;
            ImGui::SetCursorPosX((window_width - text_width) * 0.5f);
            ImGui::Text("%s", text);
        }
    }
}
