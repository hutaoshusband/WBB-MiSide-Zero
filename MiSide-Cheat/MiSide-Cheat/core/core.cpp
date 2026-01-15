#include "core.h"
#include "../hooks/hooks.h"
#include "../render/render.h"
#include <thread>
#include <chrono>

namespace core {
    
    bool Initialize(HMODULE hModule) {
        g_hModule = hModule;
        
        // Wait for game to fully initialize
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Initialize hooks (D3D11/DXGI)
        if (!hooks::Initialize()) {
            MessageBoxA(nullptr, "Failed to initialize hooks!", "WBB MiSide", MB_ICONERROR);
            return false;
        }
        
        return true;
    }
    
    void Shutdown() {
        // Restore hooks
        hooks::Shutdown();
        
        // Cleanup render
        render::Shutdown();
        
        // Small delay before unload
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    DWORD WINAPI MainThread(LPVOID lpParam) {
        // Attempt initialization
        if (!Initialize(static_cast<HMODULE>(lpParam))) {
            FreeLibraryAndExitThread(g_hModule, 0);
            return 0;
        }
        
        // Main loop - wait for unload key (END key) OR menu unload button
        while (!GetAsyncKeyState(VK_END) && !g_bShouldUnload) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup
        Shutdown();
        
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }
}
