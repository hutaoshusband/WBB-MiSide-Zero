#include "hooks.h"
#include "../core/core.h"
#include "../render/render.h"
#include "../external/minhook/MinHook.h"
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace hooks {
    
    // VMT indices for DXGI SwapChain
    constexpr int PRESENT_INDEX = 8;
    constexpr int RESIZE_BUFFERS_INDEX = 13;
    
    bool Initialize() {
        // Initialize MinHook
        if (MH_Initialize() != MH_OK) {
            return false;
        }
        
        // Create dummy device to get vtable addresses
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = 2;
        sd.BufferDesc.Height = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = GetDesktopWindow();
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        
        ID3D11Device* pDevice = nullptr;
        ID3D11DeviceContext* pContext = nullptr;
        IDXGISwapChain* pSwapChain = nullptr;
        D3D_FEATURE_LEVEL featureLevel;
        
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &sd,
            &pSwapChain,
            &pDevice,
            &featureLevel,
            &pContext
        );
        
        if (FAILED(hr)) {
            MH_Uninitialize();
            return false;
        }
        
        // Get vtable addresses
        void** pVTable = *reinterpret_cast<void***>(pSwapChain);
        void* pPresent = pVTable[PRESENT_INDEX];
        void* pResizeBuffers = pVTable[RESIZE_BUFFERS_INDEX];
        
        // Cleanup dummy device
        pSwapChain->Release();
        pContext->Release();
        pDevice->Release();
        
        // Create hooks
        if (MH_CreateHook(pPresent, &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK) {
            MH_Uninitialize();
            return false;
        }
        
        if (MH_CreateHook(pResizeBuffers, &hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers)) != MH_OK) {
            MH_Uninitialize();
            return false;
        }
        
        // Enable all hooks
        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            MH_Uninitialize();
            return false;
        }
        
        return true;
    }
    
    void Shutdown() {
        // Disable and remove all hooks
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        
        // Restore WndProc if hooked
        if (core::g_oWndProc && core::g_hWnd) {
            SetWindowLongPtrA(core::g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(core::g_oWndProc));
        }
    }
    
    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        // First time initialization
        if (!core::g_bInitialized) {
            // Get device and context from swapchain
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&core::g_pDevice)))) {
                core::g_pDevice->GetImmediateContext(&core::g_pContext);
                
                // Get window handle
                DXGI_SWAP_CHAIN_DESC desc;
                pSwapChain->GetDesc(&desc);
                core::g_hWnd = desc.OutputWindow;
                core::g_pSwapChain = pSwapChain;
                
                // Hook WndProc
                core::g_oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(core::g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));
                
                // Initialize ImGui and render system
                render::Initialize(core::g_hWnd, core::g_pDevice, core::g_pContext);
                
                core::g_bInitialized = true;
            }
        }
        
        // Render our stuff
        if (core::g_bInitialized) {
            render::BeginFrame();
            render::Render();
            render::EndFrame();
        }
        
        return oPresent(pSwapChain, SyncInterval, Flags);
    }
    
    HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        // Cleanup render target before resize
        render::InvalidateDeviceObjects();
        
        HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        
        // Only recreate render target if resize succeeded AND device/context are still valid
        if (SUCCEEDED(hr) && core::g_pDevice && core::g_pContext) {
            render::CreateDeviceObjects();
        }
        
        return hr;
    }
    
    // Track menu state to detect transitions
    static bool s_bMenuWasOpen = false;
    
    LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        bool menuOpen = render::menu::IsOpen();
        
        // Detect menu state transitions
        if (menuOpen && !s_bMenuWasOpen) {
            // Menu just opened - unlock cursor once
            ClipCursor(nullptr);
        }
        s_bMenuWasOpen = menuOpen;
        
        // Let ImGui handle input when menu is open
        if (menuOpen) {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
                return true;
            }
        }
        
        // Block game input when menu is open
        if (menuOpen) {
            // Use ImGui's software cursor
            ImGui::GetIO().MouseDrawCursor = true;
            
            switch (uMsg) {
                // Block raw input - this is what Unity uses for camera movement
                case WM_INPUT:
                    return 0;
                
                // Block all mouse messages from reaching the game
                case WM_MOUSEMOVE:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MOUSEWHEEL:
                case WM_MOUSEHWHEEL:
                case WM_XBUTTONDOWN:
                case WM_XBUTTONUP:
                case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDBLCLK:
                case WM_MBUTTONDBLCLK:
                    return 0;
                
                // Block keyboard messages from reaching the game
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_CHAR:
                    return 0;
                
                // Prevent game from changing cursor when menu is open
                case WM_SETCURSOR:
                    SetCursor(LoadCursor(nullptr, IDC_ARROW));
                    return TRUE;
            }
        } else {
            // Menu is closed - let game handle everything normally
            ImGui::GetIO().MouseDrawCursor = false;
        }
        
        return CallWindowProcA(core::g_oWndProc, hWnd, uMsg, wParam, lParam);
    }
}
