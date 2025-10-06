#pragma once

#include <imgui.h>

namespace Piccolo
{
    namespace Editor
    {
        // UI主题管理器
        class UITheme
        {
        public:
            enum class Theme
            {
                Dark,
                Light,
                Custom
            };

            static UITheme& getInstance();
            
            void setTheme(Theme theme);
            void applyCustomTheme();
            void setCustomColors();
            
            // 颜色常量
            struct Colors
            {
                // 主色调
                static const ImVec4 Primary;
                static const ImVec4 PrimaryHovered;
                static const ImVec4 PrimaryActive;
                
                // 背景色
                static const ImVec4 Background;
                static const ImVec4 BackgroundHovered;
                static const ImVec4 BackgroundActive;
                
                // 文本色
                static const ImVec4 Text;
                static const ImVec4 TextDisabled;
                static const ImVec4 TextSelected;
                
                // 边框色
                static const ImVec4 Border;
                static const ImVec4 BorderShadow;
                
                // 组件特定颜色
                static const ImVec4 Header;
                static const ImVec4 HeaderHovered;
                static const ImVec4 HeaderActive;
                
                static const ImVec4 Button;
                static const ImVec4 ButtonHovered;
                static const ImVec4 ButtonActive;
                
                static const ImVec4 FrameBg;
                static const ImVec4 FrameBgHovered;
                static const ImVec4 FrameBgActive;
                
                // 轴颜色
                static const ImVec4 AxisX;
                static const ImVec4 AxisY;
                static const ImVec4 AxisZ;
                static const ImVec4 AxisW;
            };

        private:
            Theme m_current_theme = Theme::Dark;
        };

        // UI样式助手
        class UIStyleHelper
        {
        public:
            static void pushButtonStyle(const ImVec4& color, const ImVec4& hovered_color, const ImVec4& active_color);
            static void popButtonStyle();
            
            static void pushFrameStyle(float rounding = 5.0f, float border_size = 1.0f);
            static void popFrameStyle();
            
            static void pushTextStyle(const ImVec4& color);
            static void popTextStyle();
            
            static ImVec2 calculateButtonSize(const char* text, float padding = 4.0f);
            static void centerText(const char* text);
        };
    }
}
