#pragma once
#include "ui_widgets.h"
#include "secureloader_icon.h"
#include "../texture_loader.h"
#include "../../config/config.h"
#include "../../features/features.h"
#include "../render.h"
#include "../../core/core.h"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

namespace ui {
    
    // Menu state
    inline int g_nCurrentTab = 0;
    inline float g_fMenuAlpha = 0.0f;
    
    // Sub-tabs for each main tab
    inline int g_nVisualsSubTab = 0;
    inline int g_nMiscSubTab = 0;
    
    // Unload flag
    inline bool g_bShouldUnload = false;
    
    // Logo texture
    inline ID3D11ShaderResourceView* g_pLogoTexture = nullptr;
    inline bool g_bLogoLoaded = false;
    
    // ============================================================
    // Navbar for subtabs (horizontal row of smaller tabs)
    // ============================================================
    inline void Navbar(const char* id, int* current, const char* const items[], int count) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return;
        
        float dpi_scale = scale;
        float total_width = ImGui::GetContentRegionAvail().x;
        float spacing = 5.0f * dpi_scale;
        float tab_width = (total_width - (spacing * (count - 1))) / count;
        
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
        
        for (int i = 0; i < count; i++) {
            bool active = (*current == i);
            
            if (Tab(items[i], active, ImVec2(tab_width, 28 * dpi_scale))) {
                *current = i;
            }
            
            if (i < count - 1) ImGui::SameLine();
        }
        
        ImGui::PopStyleVar();
    }
    
    // ============================================================
    // Render Watermark
    // ============================================================
    inline void RenderWatermark() {
        if (!config::g_config.menu.show_watermark) return;
        
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        
        const char* brand_text = "WallbangBros.com";
        ImVec2 brand_size = ImGui::CalcTextSize(brand_text);
        
        const float LOGO_SIZE = 24.0f;
        const float LOGO_MARGIN = 6.0f;
        const float WM_PADDING = 8.0f;
        const float RIGHT_MARGIN = 5.0f;
        const float TOP_MARGIN = 5.0f;
        
        float wm_width = brand_size.x + LOGO_SIZE + LOGO_MARGIN + WM_PADDING * 2;
        float wm_height = LOGO_SIZE + 6.0f;
        float wm_x = io.DisplaySize.x - wm_width - RIGHT_MARGIN;
        float wm_y = TOP_MARGIN;
        
        ImVec2 wm_start = ImVec2(wm_x, wm_y);
        ImVec2 wm_end = ImVec2(io.DisplaySize.x - RIGHT_MARGIN, wm_y + wm_height);
        
        draw->AddRectFilled(wm_start, wm_end, ImColor(15, 15, 20, 220), 4.0f);
        
        float text_x = wm_x + WM_PADDING;
        float text_y = wm_y + (wm_height - brand_size.y) / 2;
        draw->AddText(ImVec2(text_x, text_y), ImColor(200, 200, 200), brand_text);
        
        // Logo
        float logo_x = wm_end.x - LOGO_SIZE - 5.0f;
        float logo_y = wm_y + (wm_height - LOGO_SIZE) / 2;
        
        // Try to use loaded texture
        if (render::g_pLogoTexture) {
            // Normal UV coordinates (not flipped)
            draw->AddImage(
                (ImTextureID)render::g_pLogoTexture,
                ImVec2(logo_x, logo_y),
                ImVec2(logo_x + LOGO_SIZE, logo_y + LOGO_SIZE),
                ImVec2(0, 0), ImVec2(1, 1)  // Normal UV
            );
        } else {
            // Fallback: colored square with "W"
            draw->AddRectFilled(
                ImVec2(logo_x, logo_y),
                ImVec2(logo_x + LOGO_SIZE, logo_y + LOGO_SIZE),
                accent_color,
                4.0f
            );
            const char* logo_letter = "W";
            ImVec2 letter_size = ImGui::CalcTextSize(logo_letter);
            draw->AddText(
                ImVec2(logo_x + (LOGO_SIZE - letter_size.x) / 2, logo_y + (LOGO_SIZE - letter_size.y) / 2),
                ImColor(255, 255, 255),
                logo_letter
            );
        }
    }
    
    // ============================================================
    // Render Module List
    // ============================================================
    inline void RenderModuleList() {
        if (!config::g_config.menu.show_module_list) return;
        
        auto enabled_modules = features::GetEnabledModules();
        if (enabled_modules.empty()) return;
        
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        
        const float PADDING_X = 8.0f;
        const float LINE_HEIGHT = 18.0f;
        const float RIGHT_MARGIN = 5.0f;
        
        float y_pos = 45.0f;
        
        for (const auto& mod : enabled_modules) {
            ImVec2 text_size = ImGui::CalcTextSize(mod.name);
            float x_pos = io.DisplaySize.x - text_size.x - PADDING_X * 2 - RIGHT_MARGIN;
            
            ImVec2 bg_start = ImVec2(x_pos, y_pos);
            ImVec2 bg_end = ImVec2(io.DisplaySize.x - RIGHT_MARGIN, y_pos + LINE_HEIGHT);
            
            ImU32 bg_left = ImColor(20, 20, 25, 200);
            ImU32 bg_right = ImColor(30, 30, 35, 220);
            draw->AddRectFilledMultiColor(bg_start, bg_end, bg_left, bg_right, bg_right, bg_left);
            
            draw->AddRectFilled(
                ImVec2(io.DisplaySize.x - RIGHT_MARGIN - 2, y_pos),
                ImVec2(io.DisplaySize.x - RIGHT_MARGIN, y_pos + LINE_HEIGHT),
                accent_color
            );
            
            ImVec2 text_pos = ImVec2(x_pos + PADDING_X, y_pos + (LINE_HEIGHT - text_size.y) / 2);
            draw->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), ImColor(0, 0, 0, 150), mod.name);
            draw->AddText(text_pos, ImColor(255, 255, 255), mod.name);
            
            y_pos += LINE_HEIGHT + 1;
        }
    }
    
    // ============================================================
    // Button Widget
    // ============================================================
    inline bool Button(const char* label, ImVec2 size) {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)panel_bg_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)hover_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)accent_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ui_rounding);
        
        bool clicked = ImGui::Button(label, size);
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        
        return clicked;
    }
    
    // ============================================================
    // Tab Content: Visuals
    // ============================================================
    inline void RenderVisualsTab() {
        float dpi_scale = scale;
        float half_width = (ImGui::GetWindowWidth() - 45.0f * dpi_scale) / 2.0f;
        float available_height = ImGui::GetWindowHeight() - 130 * dpi_scale;
        
        // Subtabs
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, 58 * dpi_scale));
        const char* sub_tabs[] = { "ESP", "Chams", "World" };
        Navbar("visuals_nav", &g_nVisualsSubTab, sub_tabs, 3);
        
        float start_y = 95 * dpi_scale;
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        if (g_nVisualsSubTab == 0) {
            // ESP
            BeginChild("ESP Settings", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                CheckboxBind("Enable ESP", &config::g_config.visuals.esp);
                if (config::g_config.visuals.esp.enabled) {
                    Checkbox("Box", &config::g_config.visuals.esp_box);
                    Checkbox("Name", &config::g_config.visuals.esp_name);
                    Checkbox("Health", &config::g_config.visuals.esp_health);
                    Checkbox("Distance", &config::g_config.visuals.esp_distance);
                    SliderFloat("Max Distance", &config::g_config.visuals.esp_max_distance, 100.0f, 2000.0f, "%.0f");
                }
            }
            EndChild();
            
            ImGui::SameLine(0, 15 * dpi_scale);
            
            BeginChild("ESP Colors", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                ImGui::Text("Box Color");
                ColorPicker("##boxcol", config::g_config.visuals.esp_box_color);
            }
            EndChild();
        }
        else if (g_nVisualsSubTab == 1) {
            // Chams
            BeginChild("Chams Settings", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                CheckboxBind("Enable Chams", &config::g_config.visuals.chams);
                if (config::g_config.visuals.chams.enabled) {
                    const char* chams_types[] = { "Flat", "Textured", "Glow" };
                    Combo("Chams Type", &config::g_config.visuals.chams_type, chams_types, 3);
                    
                    Separator();
                    Checkbox("Partial Body Modulation", &config::g_config.visuals.chams_partial_body);
                    
                    if (config::g_config.visuals.chams_partial_body) {
                        ImGui::Indent(10.0f * dpi_scale);
                        Checkbox("Head", &config::g_config.visuals.chams_head);
                        Checkbox("Body", &config::g_config.visuals.chams_body);
                        Checkbox("Legs", &config::g_config.visuals.chams_legs);
                        Checkbox("Arms", &config::g_config.visuals.chams_arms);
                        ImGui::Unindent(10.0f * dpi_scale);
                    }
                }
            }
            EndChild();
            
            ImGui::SameLine(0, 15 * dpi_scale);
            
            BeginChild("Chams Colors", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                if (!config::g_config.visuals.chams_partial_body) {
                    ImGui::Text("Chams Color");
                    ColorPicker("##chamscol", config::g_config.visuals.chams_color);
                } else {
                    ImGui::Text("Head Color");
                    ColorPicker("##headcol", config::g_config.visuals.chams_head_color);
                    
                    ImGui::Text("Body Color");
                    ColorPicker("##bodycol", config::g_config.visuals.chams_body_color);
                    
                    ImGui::Text("Legs Color");
                    ColorPicker("##legscol", config::g_config.visuals.chams_legs_color);
                    
                    ImGui::Text("Arms Color");
                    ColorPicker("##armscol", config::g_config.visuals.chams_arms_color);
                }
            }
            EndChild();
        }
        else if (g_nVisualsSubTab == 2) {
            // World
            BeginChild("World Settings", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                CheckboxBind("Fullbright", &config::g_config.visuals.fullbright);
                CheckboxBind("No Fog", &config::g_config.visuals.no_fog);
                Separator();
                CheckboxBind("Crosshair", &config::g_config.visuals.crosshair);
                if (config::g_config.visuals.crosshair.enabled) {
                    const char* xhair_types[] = { "Cross", "Circle", "Dot" };
                    Combo("Type", &config::g_config.visuals.crosshair_type, xhair_types, 3);
                }
            }
            EndChild();
            
            ImGui::SameLine(0, 15 * dpi_scale);
            
            BeginChild("Crosshair Colors", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                ImGui::Text("Crosshair Color");
                ColorPicker("##xhaircol", config::g_config.visuals.crosshair_color);
            }
            EndChild();
        }
    }
    
    // ============================================================
    // Tab Content: Aimbot
    // ============================================================
    inline void RenderAimbotTab() {
        float dpi_scale = scale;
        float half_width = (ImGui::GetWindowWidth() - 45.0f * dpi_scale) / 2.0f;
        float available_height = ImGui::GetWindowHeight() - 100 * dpi_scale;
        float start_y = 65 * dpi_scale;
        
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        BeginChild("Aimbot Settings", ImVec2(half_width, available_height));
        {
            CheckboxBind("Enable Aimbot", &config::g_config.aimbot.aimbot);
            
            if (config::g_config.aimbot.aimbot.enabled) {
                SliderFloat("FOV", &config::g_config.aimbot.fov, 1.0f, 180.0f, "%.1f");
                SliderFloat("Smooth", &config::g_config.aimbot.smooth, 1.0f, 20.0f, "%.1f");
                
                Separator();
                Checkbox("Visible Only", &config::g_config.aimbot.visible_only);
                Checkbox("Team Check", &config::g_config.aimbot.team_check);
            }
        }
        EndChild();
        
        ImGui::SameLine(0, 15 * dpi_scale);
        
        BeginChild("Target Selection", ImVec2(half_width, available_height));
        {
            const char* bones[] = { "Head", "Neck", "Chest", "Stomach" };
            Combo("Target Bone", &config::g_config.aimbot.bone, bones, IM_ARRAYSIZE(bones));
        }
        EndChild();
    }
    
    // ============================================================
    // Tab Content: Misc
    // ============================================================
    inline void RenderMiscTab() {
        float dpi_scale = scale;
        float half_width = (ImGui::GetWindowWidth() - 45.0f * dpi_scale) / 2.0f;
        float available_height = ImGui::GetWindowHeight() - 130 * dpi_scale;
        
        // Subtabs
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, 58 * dpi_scale));
        const char* sub_tabs[] = { "Movement", "Player", "Game" };
        Navbar("misc_nav", &g_nMiscSubTab, sub_tabs, 3);
        
        float start_y = 95 * dpi_scale;
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        if (g_nMiscSubTab == 0) {
            // Movement
            BeginChild("Movement Settings", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                CheckboxBind("Speed Hack", &config::g_config.misc.speed_hack);
                if (config::g_config.misc.speed_hack.enabled) {
                    SliderFloat("Speed Multiplier", &config::g_config.misc.speed_multiplier, 1.0f, 5.0f, "%.1fx");
                }
                
                Separator();
                CheckboxBind("Fly Hack", &config::g_config.misc.fly_hack);
                if (config::g_config.misc.fly_hack.enabled) {
                    SliderFloat("Fly Speed", &config::g_config.misc.fly_speed, 1.0f, 50.0f, "%.0f");
                }
                
                Separator();
                CheckboxBind("NoClip", &config::g_config.misc.no_clip);
            }
            EndChild();
            
            ImGui::SameLine(0, 15 * dpi_scale);
            
            BeginChild("Movement Logic", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                ImGui::TextDisabled("How it works:");
                ImGui::BulletText("Set Key to M2 or Shift");
                ImGui::BulletText("Right-click key box to");
                ImGui::BulletText("Toggle, Hold, or Always");
                
                Separator();
                ImGui::TextDisabled("Current Binds:");
                if (config::g_config.misc.speed_hack.IsActive()) ImGui::TextColored(accent_color, "Speed Hack [ACTIVE]");
                if (config::g_config.misc.no_clip.IsActive()) ImGui::TextColored(accent_color, "NoClip [ACTIVE]");
            }
            EndChild();
        }
        else if (g_nMiscSubTab == 1) {
            // Player
            BeginChild("Player Cheats", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                CheckboxBind("God Mode", &config::g_config.misc.god_mode);
                CheckboxBind("Infinite Stamina", &config::g_config.misc.infinite_stamina);
                CheckboxBind("Infinite Ammo", &config::g_config.misc.infinite_ammo);
            }
            EndChild();
            
            ImGui::SameLine(0, 15 * dpi_scale);
            
            BeginChild("Teleport", ImVec2(half_width, available_height - 40 * dpi_scale));
            {
                CheckboxBind("Teleport", &config::g_config.misc.teleport);
            }
            EndChild();
        }
        else if (g_nMiscSubTab == 2) {
            // Game
            BeginChild("Game Modifications", ImVec2(-1, available_height - 40 * dpi_scale));
            {
                Checkbox("Debug View", &config::g_config.misc.debug_view);
                Separator();
                ImGui::TextDisabled("Game-specific features will be added here.");
                ImGui::TextDisabled("(Requires game hooking implementation)");
            }
            EndChild();
        }
    }
    
    // ============================================================
    // Tab Content: Settings
    // ============================================================
    inline void RenderSettingsTab() {
        float dpi_scale = scale;
        float half_width = (ImGui::GetWindowWidth() - 45.0f * dpi_scale) / 2.0f;
        float available_height = ImGui::GetWindowHeight() - 100 * dpi_scale;
        float start_y = 65 * dpi_scale;
        
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        BeginChild("Menu Settings", ImVec2(half_width, available_height));
        {
            ImGui::Text("Menu Key:");
            ImGui::SameLine();
            Keybind("##menukey", &config::g_config.menu.toggle_key);
            
            SliderFloat("Animation Speed", &config::g_config.menu.animation_speed, 1.0f, 15.0f, "%.0f");
            
            const char* dpi_options[] = { "75%", "100%", "125%", "150%" };
            Combo("DPI Scale", &config::g_config.menu.dpi_scale_index, dpi_options, IM_ARRAYSIZE(dpi_options));
            
            Separator();
            Checkbox("Show Watermark", &config::g_config.menu.show_watermark);
            Checkbox("Show Module List", &config::g_config.menu.show_module_list);
            
            Separator();
            ImGui::Text("Accent Color");
            ColorPicker("##accent", config::g_config.menu.accent_color);
            
            Separator();
            ImGui::Spacing();
            
            // Unload button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
            
            if (ImGui::Button("UNLOAD CHEAT", ImVec2(half_width - 30, 35 * dpi_scale))) {
                g_bShouldUnload = true;
            }
            
            ImGui::PopStyleColor(3);
            
            Separator();
            ImGui::Spacing();
            
            // Config System
            ImGui::Text("Configs:");
            static char cfgName[32] = "default";
            ImGui::InputText("##configname", cfgName, 32);
            
            if (ImGui::Button("Save Config", ImVec2((half_width - 40) / 2, 30 * dpi_scale))) {
                config::Save(cfgName);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Config", ImVec2((half_width - 40) / 2, 30 * dpi_scale))) {
                config::Load(cfgName);
            }

        }
        EndChild();
        
        ImGui::SameLine(0, 15 * dpi_scale);
        
        BeginChild("Information", ImVec2(half_width, available_height));
        {
            ImGui::TextColored(ImVec4(accent_color.Value.x, accent_color.Value.y, accent_color.Value.z, 1.0f), "WBB MiSide-Zero");
            ImGui::TextDisabled("Version: 1.0.0");
            ImGui::TextDisabled("Build: Debug");
            
            Separator();
            ImGui::TextDisabled("Hotkeys:");
            ImGui::BulletText("INSERT - Toggle Menu");
            ImGui::BulletText("END - Unload Cheat");
            
            Separator();
            ImGui::TextDisabled("Credits:");
            ImGui::BulletText("WallbangBros Team");
            ImGui::BulletText("ImGui by ocornut");
            ImGui::BulletText("MinHook by TsudaKageyu");
        }
        EndChild();
    }
    
    // ============================================================
    // Main Menu Render - 820x750
    // ============================================================
    inline void RenderMainMenu() {
        ImGuiIO& io = ImGui::GetIO();
        
        // Check unload - set flag for MainThread to handle
        if (g_bShouldUnload) {
            core::g_bShouldUnload = true;
            render::menu::g_bOpen = false;
            g_bShouldUnload = false;
            return;
        }
        
        // Note: Watermark and ModuleList are rendered in render.cpp Render() function
        
        // Update menu toggle key
        render::menu::SetToggleKey(config::g_config.menu.toggle_key);
        
        // DPI Scale
        static const float dpi_scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        int dpi_idx = config::g_config.menu.dpi_scale_index;
        if (dpi_idx < 0) dpi_idx = 0;
        if (dpi_idx > 3) dpi_idx = 3;
        float dpi_scale = dpi_scales[dpi_idx];
        
        io.FontGlobalScale = dpi_scale;
        scale = dpi_scale;
        
        // Update accent color
        SetAccentColor(ImColor(
            config::g_config.menu.accent_color[0],
            config::g_config.menu.accent_color[1],
            config::g_config.menu.accent_color[2],
            config::g_config.menu.accent_color[3]
        ));
        
        // Menu animation
        float target_alpha = render::menu::IsOpen() ? 1.0f : 0.0f;
        float delta = io.DeltaTime * config::g_config.menu.animation_speed;
        
        if (g_fMenuAlpha != target_alpha) {
            if (g_fMenuAlpha < target_alpha) {
                g_fMenuAlpha += delta;
                if (g_fMenuAlpha > 1.0f) g_fMenuAlpha = 1.0f;
            } else {
                g_fMenuAlpha -= delta;
                if (g_fMenuAlpha < 0.0f) g_fMenuAlpha = 0.0f;
            }
        }
        
        if (g_fMenuAlpha <= 0.01f) return;
        
        // Menu window - 820x750
        ImVec2 menu_size = ImVec2(820 * dpi_scale, 750 * dpi_scale);
        ImGui::SetNextWindowSize(menu_size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_fMenuAlpha);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ui_rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        
        ImGui::Begin("##WBB_MiSide_Menu", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );
        
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        
        // Main background
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg_color, ui_rounding, ImDrawFlags_RoundCornersAll);
        
        // Header bar (50px height)
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + 50 * dpi_scale), child_bg_color, ui_rounding, ImDrawFlags_RoundCornersTop);
        
        // Main tabs - CENTERED in header
        // 4 tabs * 100px + 3 * 10px spacing = 430px total
        float tabs_total_width = (4 * 100 + 3 * 10) * dpi_scale;
        float tabs_start_x = (size.x - tabs_total_width) / 2.0f;
        
        ImGui::SetCursorPos(ImVec2(tabs_start_x, 5 * dpi_scale));
        ImGui::BeginGroup();
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10 * dpi_scale, 0));
            
            if (Tab("Visuals", g_nCurrentTab == 0, ImVec2(100, 40))) g_nCurrentTab = 0;
            ImGui::SameLine();
            if (Tab("Aimbot", g_nCurrentTab == 1, ImVec2(100, 40))) g_nCurrentTab = 1;
            ImGui::SameLine();
            if (Tab("Misc", g_nCurrentTab == 2, ImVec2(100, 40))) g_nCurrentTab = 2;
            ImGui::SameLine();
            if (Tab("Settings", g_nCurrentTab == 3, ImVec2(100, 40))) g_nCurrentTab = 3;
            
            ImGui::PopStyleVar();
        }
        ImGui::EndGroup();
        
        // Tab content animation
        static int last_tab = -1;
        static float content_alpha = 1.0f;
        if (g_nCurrentTab != last_tab) {
            content_alpha = 0.0f;
            last_tab = g_nCurrentTab;
        }
        content_alpha = ImLerp(content_alpha, 1.0f, io.DeltaTime * 15.0f);
        
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, content_alpha * g_fMenuAlpha);
        
        switch (g_nCurrentTab) {
            case 0: RenderVisualsTab(); break;
            case 1: RenderAimbotTab(); break;
            case 2: RenderMiscTab(); break;
            case 3: RenderSettingsTab(); break;
        }
        
        ImGui::PopStyleVar();
        
        // Footer (30px height)
        float footer_height = 30.0f * dpi_scale;
        ImVec2 footer_pos = ImVec2(pos.x, pos.y + size.y - footer_height);
        draw->AddRectFilled(footer_pos, ImVec2(footer_pos.x + size.x, footer_pos.y + footer_height), child_bg_color, ui_rounding, ImDrawFlags_RoundCornersBottom);
        
        // Footer text
        const char* footer_left = "MiSide-Zero Edition";
        const char* footer_right = "WallbangBros | Press INSERT to toggle | END to unload";
        
        ImVec2 left_size = ImGui::CalcTextSize(footer_left);
        ImVec2 right_size = ImGui::CalcTextSize(footer_right);
        
        draw->AddText(ImVec2(footer_pos.x + 15, footer_pos.y + (footer_height - left_size.y) / 2), ImColor(150, 150, 150), footer_left);
        draw->AddText(ImVec2(footer_pos.x + size.x - right_size.x - 15, footer_pos.y + (footer_height - right_size.y) / 2), ImColor(150, 150, 150), footer_right);
        
        ImGui::End();
        
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);
    }
}
