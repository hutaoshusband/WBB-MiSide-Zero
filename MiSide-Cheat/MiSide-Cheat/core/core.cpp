#include "core.h"
#include "../hooks/hooks.h"
#include "../render/render.h"
#include "../features/features.h"
#include "../sdk/sdk.h"
#include <thread>
#include <chrono>
#include <fstream>
#include <ctime>
#include <DbgHelp.h>
#include <shlobj.h>

#pragma comment(lib, "DbgHelp.lib")

namespace core {
    
    // Crash log path
    static std::string g_CrashLogPath;
    
    // Get Documents path for crash logs
    std::string GetCrashLogPath() {
        if (g_CrashLogPath.empty()) {
            char path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
                g_CrashLogPath = std::string(path) + "\\wbb_miside_crash.log";
            } else {
                g_CrashLogPath = "C:\\wbb_miside_crash.log";
            }
        }
        return g_CrashLogPath;
    }
    
    // Write crash log entry
    void WriteCrashLog(const char* message, DWORD exceptionCode = 0, void* exceptionAddress = nullptr) {
        std::ofstream log(GetCrashLogPath(), std::ios::app);
        if (log.is_open()) {
            time_t now = time(nullptr);
            struct tm timeInfo;
            char timeBuf[64];
            if (localtime_s(&timeInfo, &now) == 0) {
                strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &timeInfo);
            } else {
                strcpy_s(timeBuf, "Unknown Time");
            }
            
            log << "[" << timeBuf << "] ";
            log << message;
            if (exceptionCode != 0) {
                log << " | ExceptionCode: 0x" << std::hex << exceptionCode;
            }
            if (exceptionAddress != nullptr) {
                log << " | Address: 0x" << std::hex << (uintptr_t)exceptionAddress;
            }
            log << std::endl;
            log.close();
        }
    }
    
    // Create minidump on crash
    LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo) {
        WriteCrashLog("FATAL CRASH DETECTED!", 
                      pExceptionInfo->ExceptionRecord->ExceptionCode,
                      pExceptionInfo->ExceptionRecord->ExceptionAddress);
        
        // Try to create a minidump
        char dumpPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, dumpPath))) {
            strcat_s(dumpPath, "\\wbb_miside_crash.dmp");
            
            HANDLE hFile = CreateFileA(dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                MINIDUMP_EXCEPTION_INFORMATION mdei;
                mdei.ThreadId = GetCurrentThreadId();
                mdei.ExceptionPointers = pExceptionInfo;
                mdei.ClientPointers = FALSE;
                
                MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, NULL, NULL);
                CloseHandle(hFile);
                
                WriteCrashLog("Minidump created at Documents\\wbb_miside_crash.dmp");
            }
        }
        
        // Let the default handler run (will show crash dialog or terminate)
        return EXCEPTION_CONTINUE_SEARCH;
    }
    
    bool Initialize(HMODULE hModule) {
        g_hModule = hModule;
        
        // Set up crash handler
        SetUnhandledExceptionFilter(CrashHandler);
        WriteCrashLog("WBB MiSide-Zero Initialized");
        
        // Wait for game to fully initialize
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Initialize hooks (D3D11/DXGI)
        if (!hooks::Initialize()) {
            MessageBoxA(nullptr, "Failed to initialize hooks!", "WBB MiSide", MB_ICONERROR);
            WriteCrashLog("FAILED: Hook initialization");
            return false;
        }
        
        WriteCrashLog("Hooks initialized successfully");
        return true;
    }
    
    void Shutdown() {
        WriteCrashLog("Shutdown initiated");
        
        // Restore hooks
        hooks::Shutdown();
        
        // Cleanup render
        render::Shutdown();
        
        // Small delay before unload
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Safe OnTick wrapper with SEH protection
    void SafeOnTick() {
        __try {
            features::OnTick();
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            static int crashCount = 0;
            crashCount++;
            
            DWORD exceptionCode = GetExceptionCode();
            WriteCrashLog("SEH Exception in OnTick", exceptionCode, nullptr);
            
            // If crashing too much, DISABLE unstable features to prevent cascade failures
            if (crashCount > 5) {
                WriteCrashLog("Too many OnTick crashes - DISABLING UNSTABLE FEATURES");
                
                // Disable features that are causing crashes
                features::DisableUnstableFeatures();
                
                // Reset crash count after disabling features
                crashCount = 0;
            }
        }
    }
    
    DWORD WINAPI MainThread(LPVOID lpParam) {
        // Attempt initialization
        if (!Initialize(static_cast<HMODULE>(lpParam))) {
            FreeLibraryAndExitThread(g_hModule, 0);
            return 0;
        }
        
        // Initialize SDK and attach THIS thread to IL2CPP runtime
        // This is CRITICAL to avoid "Fatal error in GC: Collecting from unknown thread"
        bool sdkReady = false;
        for (int i = 0; i < 30; i++) { // Try for 30 seconds
            if (sdk::Initialize()) {
                if (sdk::AttachCurrentThread()) {
                    sdkReady = true;
                    WriteCrashLog("SDK initialized and thread attached to IL2CPP runtime");
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        if (!sdkReady) {
            WriteCrashLog("WARNING: SDK or thread attachment failed - features may crash");
        }
        
        // Main loop - wait for unload key (END key) OR menu unload button
        while (!GetAsyncKeyState(VK_END) && !g_bShouldUnload) {
            // Only run OnTick if SDK is properly attached
            if (sdkReady) {
                SafeOnTick();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        // Cleanup
        Shutdown();
        
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }
}
