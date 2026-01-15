#include "render.h"
#include "../core/core.h"
#include "texture_loader.h"
#include "ui/ui_widgets.h"
#include "ui/menu.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <algorithm>
#include <cmath>

namespace render {
    
    // Render target view for resizing
    static ID3D11RenderTargetView* g_pMainRenderTargetView = nullptr;
    static bool g_bLogoInitialized = false;
    
    bool Initialize(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
        // Create ImGui context
        ImGui::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        
        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX11_Init(pDevice, pContext);
        
        // Setup style
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 8.0f;
        style.FrameRounding = 4.0f;
        style.ChildRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
        
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;
        
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;
        
        // Dark theme
        ImGui::StyleColorsDark();
        
        // Load fonts
        ImFontConfig fontConfig;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        fontConfig.PixelSnapH = true;
        
        // Default font (Segoe UI)
        g_pDefaultFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, &fontConfig);
        if (!g_pDefaultFont) {
            g_pDefaultFont = io.Fonts->AddFontDefault();
        }
        
        // Bold font
        g_pBoldFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 16.0f, &fontConfig);
        if (!g_pBoldFont) {
            g_pBoldFont = g_pDefaultFont;
        }
        
        // Large startup font (for WallbangBros text)
        ImFontConfig startupConfig;
        startupConfig.OversampleH = 1;
        startupConfig.OversampleV = 1;
        startupConfig.PixelSnapH = true;
        g_pStartupFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 72.0f, &startupConfig);
        if (!g_pStartupFont) {
            g_pStartupFont = g_pDefaultFont;
        }
        
        // Slogan font
        g_pSloganFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 22.0f, &fontConfig);
        if (!g_pSloganFont) {
            g_pSloganFont = g_pDefaultFont;
        }
        
        io.Fonts->Build();
        
        // Load logo texture
        InitializeLogo(pDevice);
        g_bLogoInitialized = true;
        
        // Create render target view
        CreateDeviceObjects();
        
        return true;
    }
    
    void Shutdown() {
        ReleaseLogo();
        InvalidateDeviceObjects();
        
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    
    void BeginFrame() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }
    
    void EndFrame() {
        ImGui::Render();
        
        if (g_pMainRenderTargetView) {
            core::g_pContext->OMSetRenderTargets(1, &g_pMainRenderTargetView, nullptr);
        }
        
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    
    void Render() {
        // Handle menu toggle key
        static bool keyPressed = false;
        if (GetAsyncKeyState(menu::g_nToggleKey) & 0x8000) {
            if (!keyPressed) {
                keyPressed = true;
                // Only toggle after startup is finished
                if (menu::g_bStartupFinished) {
                    menu::Toggle();
                }
            }
        } else {
            keyPressed = false;
        }
        
        // Render startup animation first
        if (!menu::g_bStartupFinished) {
            menu::RenderStartupAnimation();
            return;
        }
        
        // Always render watermark/module list overlay
        ui::RenderWatermark();
        ui::RenderModuleList();
        
        // Render menu if open
        if (menu::g_bOpen) {
            menu::RenderMenu();
        }
    }
    
    void InvalidateDeviceObjects() {
        if (g_pMainRenderTargetView) {
            g_pMainRenderTargetView->Release();
            g_pMainRenderTargetView = nullptr;
        }
    }
    
    void CreateDeviceObjects() {
        if (!core::g_pSwapChain || !core::g_pDevice) return;
        
        ID3D11Texture2D* pBackBuffer = nullptr;
        core::g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer) {
            core::g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pMainRenderTargetView);
            pBackBuffer->Release();
        }
    }
    
    namespace menu {
        
        bool IsOpen() {
            return g_bOpen;
        }
        
        void Toggle() {
            g_bOpen = !g_bOpen;
        }
        
        void SetToggleKey(int key) {
            g_nToggleKey = key;
        }
        
        void RenderStartupAnimation() {
            ImGuiIO& io = ImGui::GetIO();
            
            g_fStartupTime += io.DeltaTime;
            
            // Duration of startup animation
            const float duration = 5.0f;
            
            if (g_fStartupTime > duration) {
                g_bStartupFinished = true;
                return;
            }
            
            ImDrawList* draw = ImGui::GetForegroundDrawList();
            ImVec2 screenSize = io.DisplaySize;
            
            // Background dimming with fade in/out
            float bg_alpha = 0.0f;
            if (g_fStartupTime < 1.0f) {
                bg_alpha = g_fStartupTime;  // Fade in
            } else if (g_fStartupTime > duration - 1.0f) {
                bg_alpha = (duration - g_fStartupTime);  // Fade out
            } else {
                bg_alpha = 1.0f;
            }
            
            draw->AddRectFilled(ImVec2(0, 0), screenSize, ImColor(0, 0, 0, (int)(220 * bg_alpha)));
            
            // Text alpha (same timing)
            float text_alpha = bg_alpha;
            
            // Logo (centered, above text)
            const float LOGO_SIZE = 64.0f;
            float logo_x = (screenSize.x - LOGO_SIZE) / 2.0f;
            float logo_y = (screenSize.y / 2.0f) - 100.0f;
            
            if (g_pLogoTexture) {
                // Draw logo with alpha - flip UV vertically
                ImU32 logo_tint = IM_COL32(255, 255, 255, (int)(255 * text_alpha));
                draw->AddImage(
                    (ImTextureID)g_pLogoTexture,
                    ImVec2(logo_x, logo_y),
                    ImVec2(logo_x + LOGO_SIZE, logo_y + LOGO_SIZE),
                    ImVec2(0, 1), ImVec2(1, 0),  // Flipped UV
                    logo_tint
                );
            } else {
                // Fallback: colored square
                draw->AddRectFilled(
                    ImVec2(logo_x, logo_y),
                    ImVec2(logo_x + LOGO_SIZE, logo_y + LOGO_SIZE),
                    ImColor(149, 184, 6, (int)(255 * text_alpha)),
                    8.0f
                );
                // "W" letter
                const char* logo_letter = "W";
                ImVec2 letter_size = ImGui::CalcTextSize(logo_letter);
                draw->AddText(
                    ImVec2(logo_x + (LOGO_SIZE - letter_size.x) / 2, logo_y + (LOGO_SIZE - letter_size.y) / 2),
                    ImColor(255, 255, 255, (int)(255 * text_alpha)),
                    logo_letter
                );
            }
            
            // Main title: "WallbangBros"
            ImGui::PushFont(g_pStartupFont);
            const char* startupText = "WallbangBros";
            ImVec2 textSize = ImGui::CalcTextSize(startupText);
            ImVec2 textPos = ImVec2((screenSize.x - textSize.x) / 2.0f, logo_y + LOGO_SIZE + 20.0f);
            
            // Text shadow
            draw->AddText(ImVec2(textPos.x + 2, textPos.y + 2), ImColor(0, 0, 0, (int)(150 * text_alpha)), startupText);
            // Main text (accent color)
            draw->AddText(textPos, ImColor(149, 184, 6, (int)(255 * text_alpha)), startupText);
            ImGui::PopFont();
            
            // Slogan: "MiSide Zero Edition"
            ImGui::PushFont(g_pSloganFont);
            const char* sloganText = "MiSide Zero Edition";
            
            // Show loading message in final phase
            if (g_fStartupTime > 4.0f) {
                sloganText = "Initializing...";
            }
            
            ImVec2 sloganSize = ImGui::CalcTextSize(sloganText);
            ImVec2 sloganPos = ImVec2((screenSize.x - sloganSize.x) / 2.0f, textPos.y + textSize.y + 10.0f);
            
            draw->AddText(sloganPos, ImColor(200, 200, 200, (int)(200 * text_alpha)), sloganText);
            ImGui::PopFont();
            
            // Loading bar at bottom
            if (g_fStartupTime > 1.0f && g_fStartupTime < duration - 0.5f) {
                float barWidth = 300.0f;
                float barHeight = 4.0f;
                float progress = (g_fStartupTime - 1.0f) / (duration - 2.0f);
                if (progress > 1.0f) progress = 1.0f;
                
                ImVec2 barPos = ImVec2((screenSize.x - barWidth) / 2.0f, sloganPos.y + 40.0f);
                
                // Background bar
                draw->AddRectFilled(barPos, ImVec2(barPos.x + barWidth, barPos.y + barHeight), ImColor(50, 50, 50, (int)(200 * text_alpha)), 2.0f);
                
                // Progress bar
                draw->AddRectFilled(barPos, ImVec2(barPos.x + barWidth * progress, barPos.y + barHeight), ImColor(149, 184, 6, (int)(255 * text_alpha)), 2.0f);
            }
        }
        
        void RenderMenu() {
            ui::RenderMainMenu();
        }
    }
}
