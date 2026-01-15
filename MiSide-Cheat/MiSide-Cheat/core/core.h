#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace core {
    // Module handle
    inline HMODULE g_hModule = nullptr;
    
    // D3D11 Device/Context
    inline ID3D11Device* g_pDevice = nullptr;
    inline ID3D11DeviceContext* g_pContext = nullptr;
    inline IDXGISwapChain* g_pSwapChain = nullptr;
    inline ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
    
    // Window handle
    inline HWND g_hWnd = nullptr;
    inline WNDPROC g_oWndProc = nullptr;
    
    // Initialization state
    inline bool g_bInitialized = false;
    
    // Unload flag (set by menu button)
    inline bool g_bShouldUnload = false;
    
    // Initialize the cheat
    bool Initialize(HMODULE hModule);
    
    // Shutdown the cheat
    void Shutdown();
    
    // Main thread
    DWORD WINAPI MainThread(LPVOID lpParam);
}
