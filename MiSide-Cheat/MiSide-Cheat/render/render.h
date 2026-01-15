#pragma once
#include <Windows.h>
#include <d3d11.h>

// Forward declarations
struct ImFont;

namespace render {
    // Fonts
    inline ImFont* g_pDefaultFont = nullptr;
    inline ImFont* g_pBoldFont = nullptr;
    inline ImFont* g_pStartupFont = nullptr;  // Large font for startup animation
    inline ImFont* g_pSloganFont = nullptr;   // Medium font for slogan
    
    // Initialize ImGui and rendering
    bool Initialize(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    
    // Cleanup
    void Shutdown();
    
    // Frame management
    void BeginFrame();
    void EndFrame();
    void Render();
    
    // Device object management (for resize)
    void InvalidateDeviceObjects();
    void CreateDeviceObjects();
    
    namespace menu {
        // Menu state
        inline bool g_bOpen = true;  // Start open initially
        inline bool g_bStartupFinished = false;
        inline float g_fStartupTime = 0.0f;
        
        // Menu toggle key
        inline int g_nToggleKey = VK_INSERT;
        
        // Is menu open
        bool IsOpen();
        
        // Toggle menu
        void Toggle();
        
        // Set toggle key
        void SetToggleKey(int key);
        
        // Render the menu
        void RenderMenu();
        
        // Render startup animation
        void RenderStartupAnimation();
    }
}
