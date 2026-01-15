#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

// Function pointer typedefs for hooks
typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

namespace hooks {
    // Original function pointers
    inline Present_t oPresent = nullptr;
    inline ResizeBuffers_t oResizeBuffers = nullptr;
    
    // Initialize all hooks
    bool Initialize();
    
    // Remove all hooks  
    void Shutdown();
    
    // Hook functions
    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    
    // WndProc hook
    LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}
