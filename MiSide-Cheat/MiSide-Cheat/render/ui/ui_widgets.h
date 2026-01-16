#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <string>
#include <functional>
#include <Windows.h>
#include "../../bind.h"

namespace render {
    extern ImFont* g_pDefaultFont;
    extern ImFont* g_pBoldFont;
}

namespace ui {
    // ============================================================
    // Color Palette - Apple-inspired ultra dark minimalist
    // ============================================================
    inline ImColor bg_color = ImColor(0x0d, 0x0f, 0x12, 255);        // Darkest background
    inline ImColor child_bg_color = ImColor(0x16, 0x18, 0x1c, 255);  // Panel background
    inline ImColor panel_bg_color = ImColor(0x1a, 0x1c, 0x20, 255);  // Interactive elements
    
    // Accent color (lime green like CS2 cheat)
    inline ImColor accent_color = ImColor(149, 184, 6, 255);
    inline ImColor accent_hover = ImColor(169, 204, 26, 255);
    
    // Text colors
    inline ImColor text_color = ImColor(250, 250, 255, 255);
    inline ImColor text_disabled = ImColor(170, 175, 180, 255);
    
    // Borders and hover
    inline ImColor border_color = ImColor(0x40, 0x44, 0x4a, 150); // increased visibility
    inline ImColor hover_color = ImColor(0x2a, 0x2e, 0x36, 255);
    
    // UI constants
    constexpr float ui_rounding = 8.0f;
    inline float scale = 1.0f;
    
    // ============================================================
    // Animation System
    // ============================================================
    struct AnimationState {
        float alpha = 0.0f;
        float hover_alpha = 0.0f;
        bool active = false;
    };
    
    inline std::map<ImGuiID, AnimationState> animations;
    
    constexpr float ANIMATION_SPEED_ACTIVE = 12.0f;
    constexpr float ANIMATION_SPEED_HOVER = 15.0f;
    
    inline float GetAnimationState(const char* label, bool active) {
        ImGuiID id = ImGui::GetID(label);
        auto it = animations.find(id);
        if (it == animations.end()) {
            animations.insert({ id, { 0.f, 0.f, active } });
            it = animations.find(id);
        }
        float target = active ? 1.f : 0.f;
        float dt = ImGui::GetIO().DeltaTime;
        dt = (dt > 0.1f) ? 0.016f : dt;
        it->second.alpha = ImLerp(it->second.alpha, target, dt * ANIMATION_SPEED_ACTIVE);
        it->second.active = active;
        return it->second.alpha;
    }
    
    inline float GetHoverAnimation(const char* label, bool hovered) {
        ImGuiID id = ImGui::GetID(label);
        auto it = animations.find(id);
        if (it == animations.end()) {
            animations.insert({ id, { 0.f, 0.f, false } });
            it = animations.find(id);
        }
        float target = hovered ? 1.f : 0.f;
        float dt = ImGui::GetIO().DeltaTime;
        dt = (dt > 0.1f) ? 0.016f : dt;
        it->second.hover_alpha = ImLerp(it->second.hover_alpha, target, dt * ANIMATION_SPEED_HOVER);
        return it->second.hover_alpha;
    }
    
    // ============================================================
    // Set Accent Color
    // ============================================================
    inline void SetAccentColor(ImColor color) {
        accent_color = color;
        ImVec4 v = color.Value;
        accent_hover = ImColor(v.x + 0.1f, v.y + 0.1f, v.z + 0.1f, v.w);
    }
    
    // ============================================================
    // BeginChild with title - Modern transparent style
    // ============================================================
    inline void BeginChild(const char* name, ImVec2 size, bool border = true, ImGuiWindowFlags flags = 0) {
        float avail_w = ImGui::GetContentRegionAvail().x;
        float avail_h = ImGui::GetContentRegionAvail().y;
        if (size.x <= 0) size.x = avail_w;
        if (size.y <= 0) size.y = avail_h;
        
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);  // More rounded
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20 * scale, 12 * scale));  // Reduced top padding for better title centering
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * scale, 16 * scale));
        
        // Semi-transparent background
        ImColor child_bg = ImColor(35, 38, 45, 230);  // Less transparent, slightly lighter
        ImGui::PushStyleColor(ImGuiCol_ChildBg, (ImVec4)child_bg);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        
        ImGui::BeginChild(name, size, true, flags | ImGuiWindowFlags_NoScrollbar);
        ImGui::Indent(15.0f * scale);
        
        // Add extra spacing before title to move it down
        ImGui::Dummy(ImVec2(0, 4 * scale));
        
        // Render Title with accent color - vertically centered
        ImVec2 textSize = ImGui::CalcTextSize(name);
        ImVec2 cursorPos = ImGui::GetCursorPos();
        float verticalOffset = (ImGui::GetTextLineHeight() - textSize.y) / 2.0f;
        ImGui::SetCursorPosY(cursorPos.y + verticalOffset);
        
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)accent_color);
        ImGui::Text("%s", name);
        ImGui::PopStyleColor();
        
        // Subtle separator line
        ImVec2 p = ImGui::GetCursorScreenPos();
        float lineWidth = ImGui::GetContentRegionAvail().x;
        ImU32 line_col = ImColor(255, 255, 255, 20);
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(p.x, p.y + 4),
            ImVec2(p.x + lineWidth, p.y + 5),
            line_col
        );
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Dummy(ImVec2(0, 10 * scale));  // Reduced spacing after title
    }
    
    inline void EndChild() {
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
    }
    
    // ============================================================
    // Tab Button
    // ============================================================
    inline bool Tab(const char* label, bool active, ImVec2 size) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;
        
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        
        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size_arg = ImVec2(size.x * scale, size.y * scale);
        if (size.x == 0.0f) size_arg.x = label_size.x + 30 * scale;
        if (size.y == 0.0f) size_arg.y = label_size.y + 18 * scale;
        
        const ImRect bb(pos, ImVec2(pos.x + size_arg.x, pos.y + size_arg.y));
        ImGui::ItemSize(size_arg, 0);
        if (!ImGui::ItemAdd(bb, id)) return false;
        
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        
        float anim = GetAnimationState(label, active);
        float hover_anim = GetHoverAnimation(label, hovered);
        
        // Background gradient with animation
        if (active) {
            ImColor grad_start = ImColor(
                (int)(accent_color.Value.x * 255 * 0.3f * anim),
                (int)(accent_color.Value.y * 255 * 0.3f * anim),
                (int)(accent_color.Value.z * 255 * 0.3f * anim),
                (int)(200 * anim)
            );
            ImColor grad_end = ImColor(
                (int)(accent_color.Value.x * 255 * 0.15f * anim),
                (int)(accent_color.Value.y * 255 * 0.15f * anim),
                (int)(accent_color.Value.z * 255 * 0.15f * anim),
                (int)(180 * anim)
            );
            
            window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max, grad_start, grad_start, grad_end, grad_end);
            
            // Active indicator line with animation
            float indicator_width = (size_arg.x - 20) * anim;
            float indicator_x = bb.Min.x + 10 + (size_arg.x - 20 - indicator_width) / 2;
            window->DrawList->AddRectFilled(
                ImVec2(indicator_x, bb.Max.y - 2),
                ImVec2(indicator_x + indicator_width, bb.Max.y),
                accent_color, 2.f
            );
        } else if (hovered) {
            ImColor grad_start = ImColor(
                (int)(accent_color.Value.x * 255 * 0.6f * hover_anim),
                (int)(accent_color.Value.y * 255 * 0.6f * hover_anim),
                (int)(accent_color.Value.z * 255 * 0.6f * hover_anim),
                (int)(200 * hover_anim)
            );
            ImColor grad_end = ImColor(
                (int)(accent_color.Value.x * 255 * 0.3f * hover_anim),
                (int)(accent_color.Value.y * 255 * 0.3f * hover_anim),
                (int)(accent_color.Value.z * 255 * 0.3f * hover_anim),
                (int)(180 * hover_anim)
            );
            
            window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max, grad_start, grad_start, grad_end, grad_end);
        }
        
        // Text with animation
        ImColor text_col = active ? accent_color : text_disabled;
        if (!active && hovered) {
            text_col = ImColor(
                (int)ImLerp(text_disabled.Value.x * 255, text_color.Value.x * 255, hover_anim),
                (int)ImLerp(text_disabled.Value.y * 255, text_color.Value.y * 255, hover_anim),
                (int)ImLerp(text_disabled.Value.z * 255, text_color.Value.z * 255, hover_anim)
            );
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)text_col);
        ImGui::RenderTextClipped(bb.Min, bb.Max, label, NULL, &label_size, ImVec2(0.5f, 0.5f), &bb);
        ImGui::PopStyleColor();
        
        return pressed;
    }
    
    // ============================================================
    // Checkbox with animation
    // ============================================================
    inline bool Checkbox(const char* label, bool* v) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;
        
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        
        const float square_sz = 14.0f * scale;
        const ImVec2 pos = window->DC.CursorPos;
        const ImRect total_bb(pos, ImVec2(pos.x + square_sz + 8 + label_size.x, pos.y + ImMax(square_sz, label_size.y) + 4));
        
        ImGui::ItemSize(total_bb);
        if (!ImGui::ItemAdd(total_bb, id)) return false;
        
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed) {
            *v = !(*v);
            ImGui::MarkItemEdited(id);
        }
        
        const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));
        
        float check_anim = GetAnimationState(label, *v);
        float hover_anim = GetHoverAnimation(label, hovered);
        
        // Background
        ImColor bg_col = hovered ? hover_color : panel_bg_color;
        window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, bg_col, ui_rounding);
        
        // Border
        ImColor border_col = (*v || hovered) ? accent_color : border_color;
        window->DrawList->AddRect(check_bb.Min, check_bb.Max, border_col, ui_rounding, 0, 1.0f);
        
        // Check fill (animated)
        if (check_anim > 0.01f) {
            ImColor check_col = ImColor(
                (int)(accent_color.Value.x * 255),
                (int)(accent_color.Value.y * 255),
                (int)(accent_color.Value.z * 255),
                (int)(255 * check_anim)
            );
            window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, check_col, ui_rounding);
        }
        
        // Label
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)(hovered ? text_color : text_disabled));
        ImGui::RenderText(ImVec2(check_bb.Max.x + 15 * scale, check_bb.Min.y + (square_sz - label_size.y) / 2.0f), label);
        ImGui::PopStyleColor();
        
        return pressed;
    }
    
    // ============================================================
    // Slider Float
    // ============================================================
    inline bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f") {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)panel_bg_color);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)hover_color);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)hover_color);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)accent_color);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, (ImVec4)accent_hover);
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ui_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, ui_rounding);
        
        bool changed = ImGui::SliderFloat(label, v, v_min, v_max, format);
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
        
        return changed;
    }
    
    // ============================================================
    // Slider Int
    // ============================================================
    inline bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d") {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)panel_bg_color);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)hover_color);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)hover_color);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)accent_color);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, (ImVec4)accent_hover);
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ui_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, ui_rounding);
        
        bool changed = ImGui::SliderInt(label, v, v_min, v_max, format);
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
        
        return changed;
    }
    
    // ============================================================
    // Combo Box
    // ============================================================
    inline bool Combo(const char* label, int* current_item, const char* const items[], int items_count) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)panel_bg_color);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)hover_color);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)panel_bg_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)hover_color);
        ImGui::PushStyleColor(ImGuiCol_Header, (ImVec4)accent_color);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImVec4)accent_hover);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, (ImVec4)child_bg_color);
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ui_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, ui_rounding);
        
        bool changed = ImGui::Combo(label, current_item, items, items_count);
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(7);
        
        return changed;
    }
    
    // ============================================================
    // Keybind Widget
    // ============================================================
    inline const char* VirtualKeyToString(int key) {
        static char name[128];
        if (key == 0) return "None";
        
        LONG lScan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
        if (GetKeyNameTextA(lScan << 16, name, sizeof(name)) > 0)
            return name;
        
        switch (key) {
            case VK_LBUTTON: return "M1";
            case VK_RBUTTON: return "M2";
            case VK_MBUTTON: return "M3";
            case VK_XBUTTON1: return "M4";
            case VK_XBUTTON2: return "M5";
            default: return "Key";
        }
    }
    
    inline void Keybind(const char* label, int* key, int* mode = nullptr) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;
        
        const ImGuiID id = window->GetID(label);
        
        char buf[32];
        if (*key == 0) strcpy(buf, "[-]");
        else strcpy(buf, VirtualKeyToString(*key));
        
        ImVec2 label_size = ImGui::CalcTextSize(buf);
        ImVec2 size = ImVec2(label_size.x + 16, label_size.y + 8);
        if (size.x < 80.0f) size.x = 80.0f;
        
        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        
        ImGui::ItemSize(size);
        if (!ImGui::ItemAdd(bb, id)) return;
        
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        
        static bool waiting_for_key = false;
        static ImGuiID waiting_id = 0;
        
        if (pressed && !waiting_for_key) {
            waiting_for_key = true;
            waiting_id = id;
        }
        
        if (waiting_for_key && waiting_id == id) {
            strcpy(buf, "[PRESS KEY]");
            
            for (int i = 0; i < 256; i++) {
                if (GetAsyncKeyState(i) & 0x8000) {
                    if (i == VK_ESCAPE) {
                        *key = 0;
                    } else {
                        *key = i;
                    }
                    waiting_for_key = false;
                    waiting_id = 0;
                    break;
                }
            }
            
            // Mouse buttons
            if (ImGui::IsMouseClicked(0)) { *key = VK_LBUTTON; waiting_for_key = false; }
            if (ImGui::IsMouseClicked(1)) { *key = VK_RBUTTON; waiting_for_key = false; }
            if (ImGui::IsMouseClicked(2)) { *key = VK_MBUTTON; waiting_for_key = false; }
        }
        
        // Render
        ImColor bg_col = hovered ? hover_color : panel_bg_color;
        if (waiting_for_key && waiting_id == id) bg_col = accent_color;
        
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, 4.f);
        window->DrawList->AddRect(bb.Min, bb.Max, border_color, 4.f);
        
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)text_color);
        ImGui::RenderTextClipped(bb.Min, bb.Max, buf, NULL, NULL, ImVec2(0.5f, 0.5f), &bb);
        ImGui::PopStyleColor();
        
        // Mode popup on right click
        if (mode && hovered && ImGui::IsMouseClicked(1)) {
            ImGui::OpenPopup(("##mode_" + std::string(label)).c_str());
        }
        
        if (mode && ImGui::BeginPopup(("##mode_" + std::string(label)).c_str())) {
            ImGui::TextDisabled("Trigger Mode");
            ImGui::Separator();
            if (ImGui::Selectable("Hold", *mode == 0)) *mode = 0;
            if (ImGui::Selectable("Toggle", *mode == 1)) *mode = 1;
            if (ImGui::Selectable("Always On", *mode == 2)) *mode = 2;
            ImGui::EndPopup();
        }
    }

    inline bool CheckboxBind(const char* label, config::Bind* bind) {
        float draw_scale = scale;
        bool changed = Checkbox(label, &bind->enabled);
        
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 110 * draw_scale);
        
        Keybind(("##key_" + std::string(label)).c_str(), &bind->key, &bind->mode);
        
        return changed;
    }
    
    // ============================================================
    // Separator
    // ============================================================
    inline void Separator() {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p.x, p.y),
            ImVec2(p.x + width, p.y),
            border_color
        );
        ImGui::Dummy(ImVec2(0, 8));
    }
    
    // ============================================================
    // Color Picker
    // ============================================================
    inline bool ColorPicker(const char* label, float col[4]) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ui_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, ui_rounding);
        
        bool changed = ImGui::ColorEdit4(label, col, 
            ImGuiColorEditFlags_NoInputs | 
            ImGuiColorEditFlags_AlphaBar | 
            ImGuiColorEditFlags_AlphaPreview);
        
        ImGui::PopStyleVar(2);
        return changed;
    }
    
    // Forward declaration for menu render
    void RenderMainMenu();
}
