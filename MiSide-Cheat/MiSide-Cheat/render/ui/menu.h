#pragma once
#include "ui_widgets.h"
#include "secureloader_icon.h"
#include "../texture_loader.h"
#include "../../config/config.h"
#include "../../features/features.h"
#include "../../features/debug_draw.h"
#include "../../sdk/sdk.h"
#include "../render.h"
#include "../../core/core.h"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

namespace ui {
    
    // Menu state
    inline int g_nCurrentTab = 0;
    inline std::vector<std::string> g_vecConfigFiles;
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
        float full_width = ImGui::GetWindowWidth() - 30.0f * dpi_scale;
        float available_height = ImGui::GetWindowHeight() - 130 * dpi_scale;
        
        // Subtabs
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, 58 * dpi_scale));
        const char* sub_tabs[] = { "ESP", "Chams", "World" };
        Navbar("visuals_nav", &g_nVisualsSubTab, sub_tabs, 3);
        
        float start_y = 95 * dpi_scale;
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        if (g_nVisualsSubTab == 0) {
            // ESP
            BeginChild("ESP Settings", ImVec2(full_width, available_height));
            {
                CheckboxBind("Enable ESP", &config::g_config.visuals.esp);
                if (config::g_config.visuals.esp.enabled) {
                    ImGui::Indent();
                    Checkbox("Box", &config::g_config.visuals.esp_box);
                    Checkbox("Name", &config::g_config.visuals.esp_name);
                    Checkbox("Distance", &config::g_config.visuals.esp_distance);
                    SliderFloat("Max Distance", &config::g_config.visuals.esp_max_distance, 100.0f, 2000.0f, "%.0f");
                    ImGui::Unindent();
                }
                
                Separator();
                ImGui::Text("ESP Colors");
                ColorPicker("##boxcol", config::g_config.visuals.esp_box_color);
            }
            EndChild();
        }
        else if (g_nVisualsSubTab == 1) {
            // Chams
            BeginChild("Chams Settings", ImVec2(full_width, available_height));
            {
                CheckboxBind("Enable Chams", &config::g_config.visuals.chams);
                if (config::g_config.visuals.chams.enabled) {
                    ImGui::Indent();
                    const char* chams_types[] = { "Flat", "Textured", "Glow" };
                    Combo("Chams Type", &config::g_config.visuals.chams_type, chams_types, 3);
                    ImGui::Unindent();
                }
                
                Separator();
                ImGui::Text("Chams Color");
                ColorPicker("##chamscol", config::g_config.visuals.chams_color);
            }
            EndChild();
        }
        else if (g_nVisualsSubTab == 2) {
            // World
            BeginChild("World Settings", ImVec2(full_width, available_height));
            {
                Checkbox("Path Prediction (Movement)", &config::g_config.visuals.path_prediction);
                if (config::g_config.visuals.path_prediction) {
                     ImGui::TextDisabled("Predicts movement of platforms and projectiles.");
                }
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
        float full_width = ImGui::GetWindowWidth() - 30.0f * dpi_scale;
        float available_height = ImGui::GetWindowHeight() - 130 * dpi_scale;
        
        // Subtabs
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, 58 * dpi_scale));
        const char* sub_tabs[] = { "Movement", "Player", "Game", "Debug" };
        Navbar("misc_nav", &g_nMiscSubTab, sub_tabs, 4);
        
        float start_y = 95 * dpi_scale;
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        if (g_nMiscSubTab == 0) {
            // Movement
            BeginChild("Movement Settings", ImVec2(full_width, available_height));
            {
                CheckboxBind("Speed Hack", &config::g_config.misc.speed_hack);
                if (config::g_config.misc.speed_hack.enabled) {
                    ImGui::Indent();
                    SliderFloat("Speed Multiplier", &config::g_config.misc.speed_multiplier, 1.0f, 5.0f, "%.1fx");
                    ImGui::Unindent();
                }
                
                Separator();
                CheckboxBind("Fly Hack", &config::g_config.misc.fly_hack);
                if (config::g_config.misc.fly_hack.enabled) {
                    ImGui::Indent();
                    SliderFloat("Fly Speed", &config::g_config.misc.fly_speed, 1.0f, 50.0f, "%.0f");
                    ImGui::Unindent();
                }
                
                Separator();
                CheckboxBind("NoClip", &config::g_config.misc.no_clip);
            }
            EndChild();
        }
        else if (g_nMiscSubTab == 1) {
            // Player
            BeginChild("Player Cheats", ImVec2(full_width, available_height));
            {
                ImGui::TextDisabled("No player features available yet.");
            }
            EndChild();
        }
        else if (g_nMiscSubTab == 2) {
            // Game
            BeginChild("Game Modifications", ImVec2(full_width, available_height));
            {
                // Mita Speed Hack
                Checkbox("Mita Speed Hack", &config::g_config.misc.mita_speed_enabled);
                if (config::g_config.misc.mita_speed_enabled) {
                    ImGui::Indent();
                    SliderFloat("Speed Multiplier", &config::g_config.misc.mita_speed, 1.0f, 20.0f, "%.1f");
                    ImGui::Unindent();
                }

                Separator();
                Checkbox("Debug View", &config::g_config.misc.debug_view);
            }
            EndChild();
        }
        else if (g_nMiscSubTab == 3) {
            // Debug
            BeginChild("Debug Tools", ImVec2(full_width, available_height));
            {
                // Debug View Toggle Button
                if (Button(config::g_config.misc.debug_view ? "Close Debug View" : "Open Debug View", ImVec2(full_width - 30, 35 * dpi_scale))) {
                    config::g_config.misc.debug_view = !config::g_config.misc.debug_view;
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Debug Draw Hooks
                Checkbox("Enable Debug Hooks", &config::g_config.misc.debug_draw_hooks);
                if (config::g_config.misc.debug_draw_hooks) {
                    ImGui::Indent();
                    Checkbox("Render Debug Lines", &config::g_config.misc.debug_draw_render);
                    ImGui::Spacing();
                    SliderInt("Max Lines", &config::g_config.misc.debug_draw_max_lines, 100, 2000, "%d");
                    SliderInt("Max Rays", &config::g_config.misc.debug_draw_max_rays, 100, 2000, "%d");
                    ImGui::Unindent();
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Statistics (compact)
                auto debugStats = features::debug_draw::GetStats();
                ImGui::Text("Hooks: %s | Lines: %d | Rays: %d", 
                    debugStats.hook_active ? "ON" : "OFF",
                    debugStats.lines_captured,
                    debugStats.rays_captured);
                
                ImGui::Spacing();
                if (Button("Clear Statistics", ImVec2(full_width - 30, 30 * dpi_scale))) {
                    features::debug_draw::ClearStats();
                }
            }
            EndChild();
        }
    }
    
    // ============================================================
    // Tab Content: Config (NEW - like CS2 Cheat)
    // ============================================================
    inline void RefreshConfigList() {
        g_vecConfigFiles.clear();
        std::string dir = config::GetConfigDirectory();
        if (dir.empty()) return;
        
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA((dir + "\\*.cfg").c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    g_vecConfigFiles.push_back(fd.cFileName);
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
    }
    
    inline void RenderConfigTab() {
        float dpi_scale = scale;
        float half_width = (ImGui::GetWindowWidth() - 45.0f * dpi_scale) / 2.0f;
        float available_height = ImGui::GetWindowHeight() - 100 * dpi_scale;
        float start_y = 65 * dpi_scale;
        
        static bool first_run = true;
        if (first_run) {
            RefreshConfigList();
            first_run = false;
        }
        
        static char cfgName[64] = "default";
        static int selectedConfig = 0;
        
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        // Left Panel - Config Management
        BeginChild("Config Management", ImVec2(half_width, available_height));
        {
            ImGui::Text("Config Name:");
            ImGui::InputText("##cfgname", cfgName, 64);
            
            ImGui::Spacing();
            Separator();
            ImGui::Spacing();
            
            if (Button("Create New", ImVec2(half_width - 30, 30 * dpi_scale))) {
                if (strlen(cfgName) > 0) {
                    config::Save(cfgName);
                    RefreshConfigList();
                }
            }
            
            ImGui::Spacing();
            
            if (Button("Load Selected", ImVec2(half_width - 30, 30 * dpi_scale))) {
                if (selectedConfig >= 0 && selectedConfig < (int)g_vecConfigFiles.size()) {
                    std::string name = g_vecConfigFiles[selectedConfig];
                    // Remove .cfg extension
                    if (name.size() > 4) name = name.substr(0, name.size() - 4);
                    config::Load(name.c_str());
                }
            }
            
            ImGui::Spacing();
            
            if (Button("Save Selected", ImVec2(half_width - 30, 30 * dpi_scale))) {
                if (selectedConfig >= 0 && selectedConfig < (int)g_vecConfigFiles.size()) {
                    std::string name = g_vecConfigFiles[selectedConfig];
                    if (name.size() > 4) name = name.substr(0, name.size() - 4);
                    config::Save(name.c_str());
                }
            }
            
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
            if (Button("Delete Selected", ImVec2(half_width - 30, 30 * dpi_scale))) {
                if (selectedConfig >= 0 && selectedConfig < (int)g_vecConfigFiles.size()) {
                    std::string dir = config::GetConfigDirectory();
                    std::string path = dir + "\\" + g_vecConfigFiles[selectedConfig];
                    DeleteFileA(path.c_str());
                    RefreshConfigList();
                    if (selectedConfig >= (int)g_vecConfigFiles.size()) {
                        selectedConfig = (int)g_vecConfigFiles.size() - 1;
                    }
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::Spacing();
            Separator();
            ImGui::Spacing();
            
            if (Button("Refresh List", ImVec2(half_width - 30, 30 * dpi_scale))) {
                RefreshConfigList();
            }
            
            ImGui::Spacing();
            
            if (Button("Open Folder", ImVec2(half_width - 30, 30 * dpi_scale))) {
                std::string dir = config::GetConfigDirectory();
                ShellExecuteA(NULL, "open", dir.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
        EndChild();
        
        ImGui::SameLine(0, 15 * dpi_scale);
        
        // Right Panel - Config List
        BeginChild("Available Configs", ImVec2(half_width, available_height));
        {
            ImGui::TextColored(ImVec4(accent_color.Value.x, accent_color.Value.y, accent_color.Value.z, 1.0f), "Available Configs");
            ImGui::Spacing();
            Separator();
            ImGui::Spacing();
            
            if (g_vecConfigFiles.empty()) {
                ImGui::TextDisabled("No configs found.");
                ImGui::TextDisabled("Create one using the left panel.");
            } else {
                for (int i = 0; i < (int)g_vecConfigFiles.size(); i++) {
                    bool is_selected = (selectedConfig == i);
                    
                    if (Tab(g_vecConfigFiles[i].c_str(), is_selected, ImVec2(half_width - 30, 35 * dpi_scale))) {
                        selectedConfig = i;
                    }
                }
            }
        }
        EndChild();
    }
    
    // ============================================================
    // Tab Content: Settings
    // ============================================================
    inline void RenderSettingsTab() {
        float dpi_scale = scale;
        float full_width = ImGui::GetWindowWidth() - 30.0f * dpi_scale;
        float available_height = ImGui::GetWindowHeight() - 100 * dpi_scale;
        float start_y = 65 * dpi_scale;
        
        ImGui::SetCursorPos(ImVec2(15 * dpi_scale, start_y));
        
        BeginChild("Menu Settings", ImVec2(full_width, available_height));
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
            
            if (ImGui::Button("UNLOAD CHEAT", ImVec2(full_width - 30, 35 * dpi_scale))) {
                g_bShouldUnload = true;
            }
            
            ImGui::PopStyleColor(3);
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
        
        // Menu animation with fade and scale
        static float g_fMenuScale = 0.0f;
        static bool g_bMenuInitialized = false;
        static bool g_bLastMenuOpenState = false;
        bool menu_is_open = render::menu::IsOpen();
        
        // Initialize menu state on first run
        if (!g_bMenuInitialized) {
            g_bLastMenuOpenState = menu_is_open;
            g_bMenuInitialized = true;
            // Start with menu closed animation state
            g_fMenuAlpha = 0.0f;
            g_fMenuScale = 0.95f;
        }
        
        // Detect menu state change to trigger animation
        if (menu_is_open != g_bLastMenuOpenState) {
            g_bLastMenuOpenState = menu_is_open;
            // Reset animation when state changes - start from current alpha
            // This ensures animation plays every time menu opens/closes
        }
        
        float target_alpha = menu_is_open ? 1.0f : 0.0f;
        float target_scale = menu_is_open ? 1.0f : 0.95f;
        float menu_anim_speed = config::g_config.menu.animation_speed;
        
        // Ensure minimum animation speed to avoid division by zero or no animation
        if (menu_anim_speed < 0.1f) menu_anim_speed = 0.1f;
        
        // Simple linear interpolation for animation
        float alpha_diff = target_alpha - g_fMenuAlpha;
        float scale_diff = target_scale - g_fMenuScale;
        
        g_fMenuAlpha += alpha_diff * io.DeltaTime * menu_anim_speed;
        g_fMenuScale += scale_diff * io.DeltaTime * menu_anim_speed * 0.8f;
        
        // Clamp values
        if (g_fMenuAlpha < 0.01f) g_fMenuAlpha = 0.0f;
        if (g_fMenuAlpha > 0.99f) g_fMenuAlpha = 1.0f;
        
        // Return early if menu is closed (don't render)
        if (!menu_is_open) return;
        
        // Menu window - 820x750 with scale animation
        ImVec2 base_size = ImVec2(820 * dpi_scale, 750 * dpi_scale);
        ImVec2 menu_size = ImVec2(base_size.x * g_fMenuScale, base_size.y * g_fMenuScale);
        ImGui::SetNextWindowSize(menu_size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        
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
        // 5 tabs * 90px + 4 * 10px spacing = 490px total
        float tabs_total_width = (5 * 90 + 4 * 10) * dpi_scale;
        float tabs_start_x = (size.x - tabs_total_width) / 2.0f;
        
        ImGui::SetCursorPos(ImVec2(tabs_start_x, 5 * dpi_scale));
        ImGui::BeginGroup();
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10 * dpi_scale, 0));
            
            if (Tab("Visuals", g_nCurrentTab == 0, ImVec2(90, 40))) g_nCurrentTab = 0;
            ImGui::SameLine();
            if (Tab("Aimbot", g_nCurrentTab == 1, ImVec2(90, 40))) g_nCurrentTab = 1;
            ImGui::SameLine();
            if (Tab("Misc", g_nCurrentTab == 2, ImVec2(90, 40))) g_nCurrentTab = 2;
            ImGui::SameLine();
            if (Tab("Config", g_nCurrentTab == 3, ImVec2(90, 40))) g_nCurrentTab = 3;
            ImGui::SameLine();
            if (Tab("Settings", g_nCurrentTab == 4, ImVec2(90, 40))) g_nCurrentTab = 4;
            
            ImGui::PopStyleVar();
        }
        ImGui::EndGroup();
        
        // Tab content animation with slide and fade
        static int last_tab = -1;
        static float content_alpha = 1.0f;
        static float content_offset = 0.0f;
        static int tab_direction = 0;  // -1 for left, 1 for right
        
        if (g_nCurrentTab != last_tab) {
            content_alpha = 0.0f;
            tab_direction = (g_nCurrentTab > last_tab) ? 1 : -1;
            content_offset = 30.0f * tab_direction * dpi_scale;
            last_tab = g_nCurrentTab;
        }
        
        float anim_speed = config::g_config.menu.animation_speed * 1.5f;
        content_alpha = ImLerp(content_alpha, 1.0f, io.DeltaTime * anim_speed);
        content_offset = ImLerp(content_offset, 0.0f, io.DeltaTime * anim_speed);
        
        // Apply slide offset
        ImVec2 original_cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(original_cursor.x + content_offset, original_cursor.y));
        
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, content_alpha * g_fMenuAlpha);
        
        switch (g_nCurrentTab) {
            case 0: RenderVisualsTab(); break;
            case 1: RenderAimbotTab(); break;
            case 2: RenderMiscTab(); break;
            case 3: RenderConfigTab(); break;
            case 4: RenderSettingsTab(); break;
        }
        
        ImGui::PopStyleVar();
        
        // Restore cursor position after slide animation
        ImGui::SetCursorPos(original_cursor);
        
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
