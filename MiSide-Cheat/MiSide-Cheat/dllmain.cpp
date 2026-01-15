// WBB MiSide-Zero Cheat
// DLL Entry Point

#include "pch.h"
#include "core/core.h"
#include <thread>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Disable thread library calls for optimization
        DisableThreadLibraryCalls(hModule);
        
        // Create main thread
        HANDLE hThread = CreateThread(
            nullptr,
            0,
            core::MainThread,
            hModule,
            0,
            nullptr
        );
        
        if (hThread)
        {
            CloseHandle(hThread);
        }
    }
    
    return TRUE;
}
