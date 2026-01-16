#include "debug_draw.h"
#include "../external/minhook/include/MinHook.h"
#include <chrono>
#include <unordered_map>

namespace features {
namespace debug_draw {

    // ============================================================
    // Original function pointers
    // ============================================================
    using DebugLine_Func = void(*)(sdk::Vector3, sdk::Vector3, float, float, float, float, float, bool, void*);
    using DebugRay_Func = void(*)(sdk::Vector3, sdk::Vector3, float, float, float, float, float, bool, void*);
    
    static DebugLine_Func original_DebugLine = nullptr;
    static DebugRay_Func original_DebugRay = nullptr;

    // ============================================================
    // Storage for captured debug draws
    // ============================================================
    static std::vector<DebugLine> captured_lines;
    static std::vector<DebugRay> captured_rays;
    static std::mutex data_mutex;
    
    // Statistics
    static DebugDrawStats stats;
    
    // State
    static bool hooks_enabled = false;
    static bool initialized = false;

    // ============================================================
    // Hook Function: Debug.DrawLine
    // ============================================================
    void hk_DebugLine(sdk::Vector3 start, sdk::Vector3 end, float r, float g, float b, float a, float duration, bool depthTest, void* method) {
        // Call original function first
        if (original_DebugLine) {
            original_DebugLine(start, end, r, g, b, a, duration, depthTest, method);
        }

        // Capture the line
        std::lock_guard<std::mutex> lock(data_mutex);
        stats.lines_captured++;
        captured_lines.push_back({ start, end, r, g, b, a, duration, depthTest });
        
        // Limit buffer size (keep last 1000 lines)
        if (captured_lines.size() > 1000) {
            captured_lines.erase(captured_lines.begin());
        }
    }

    // ============================================================
    // Hook Function: Debug.DrawRay
    // ============================================================
    void hk_DebugRay(sdk::Vector3 start, sdk::Vector3 dir, float r, float g, float b, float a, float duration, bool depthTest, void* method) {
        // Call original function first
        if (original_DebugRay) {
            original_DebugRay(start, dir, r, g, b, a, duration, depthTest, method);
        }

        // Capture the ray
        std::lock_guard<std::mutex> lock(data_mutex);
        stats.rays_captured++;
        captured_rays.push_back({ start, dir, r, g, b, a, duration, depthTest });
        
        // Limit buffer size (keep last 1000 rays)
        if (captured_rays.size() > 1000) {
            captured_rays.erase(captured_rays.begin());
        }
    }

    // ============================================================
    // Initialization
    // ============================================================
    bool Initialize() {
        if (initialized) {
            return true;
        }

        // Initialize MinHook
        MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
            return false;
        }

        initialized = true;
        return true;
    }

    // ============================================================
    // Shutdown
    // ============================================================
    void Shutdown() {
        DisableHooks();
        
        if (initialized) {
            MH_Uninitialize();
            initialized = false;
        }
    }

    // ============================================================
    // Enable Hooks
    // ============================================================
    void EnableHooks() {
        if (!initialized) {
            Initialize();
        }

        if (hooks_enabled) {
            return;
        }

        // Get function addresses from IL2CPP
        // Debug.DrawLine (RVA: 0x1911220 from dump.cs)
        void* debugLineAddr = (void*)0x1801911220; // Adjust base address as needed
        
        // Debug.DrawRay (RVA: 0x1911480 from dump.cs)
        void* debugRayAddr = (void*)0x1801911480; // Adjust base address as needed

        // Note: These addresses might need adjustment based on the actual module base
        // We'll need to get the module base address at runtime
        
        // For now, we'll skip the hook if addresses are invalid
        // TODO: Implement proper address resolution using the dump addresses
        
        // Attempt to hook Debug.DrawLine
        MH_STATUS status1 = MH_CreateHook(debugLineAddr, &hk_DebugLine, (LPVOID*)&original_DebugLine);
        if (status1 == MH_OK) {
            MH_EnableHook(debugLineAddr);
        }

        // Attempt to hook Debug.DrawRay
        MH_STATUS status2 = MH_CreateHook(debugRayAddr, &hk_DebugRay, (LPVOID*)&original_DebugRay);
        if (status2 == MH_OK) {
            MH_EnableHook(debugRayAddr);
        }

        if (status1 == MH_OK || status2 == MH_OK) {
            hooks_enabled = true;
            stats.hook_active = true;
        }
    }

    // ============================================================
    // Disable Hooks
    // ============================================================
    void DisableHooks() {
        if (!hooks_enabled) {
            return;
        }

        void* debugLineAddr = (void*)0x1801911220;
        void* debugRayAddr = (void*)0x1801911480;

        MH_DisableHook(debugLineAddr);
        MH_DisableHook(debugRayAddr);

        hooks_enabled = false;
        stats.hook_active = false;
    }

    // ============================================================
    // Check if hooks are enabled
    // ============================================================
    bool IsEnabled() {
        return hooks_enabled;
    }

    // ============================================================
    // Get captured lines (thread-safe)
    // ============================================================
    std::vector<DebugLine> GetLines() {
        std::lock_guard<std::mutex> lock(data_mutex);
        return captured_lines;
    }

    // ============================================================
    // Get captured rays (thread-safe)
    // ============================================================
    std::vector<DebugRay> GetRays() {
        std::lock_guard<std::mutex> lock(data_mutex);
        return captured_rays;
    }

    // ============================================================
    // Get statistics
    // ============================================================
    DebugDrawStats GetStats() {
        std::lock_guard<std::mutex> lock(data_mutex);
        return stats;
    }

    // ============================================================
    // Clear statistics
    // ============================================================
    void ClearStats() {
        std::lock_guard<std::mutex> lock(data_mutex);
        stats.lines_captured = 0;
        stats.rays_captured = 0;
        stats.gizmos_called = 0;
    }

    // ============================================================
    // Internal callback for debug lines (called from hook)
    // ============================================================
    void OnDebugLine(sdk::Vector3 start, sdk::Vector3 end, float r, float g, float b, float a, float duration, bool depthTest) {
        std::lock_guard<std::mutex> lock(data_mutex);
        stats.lines_captured++;
        captured_lines.push_back({ start, end, r, g, b, a, duration, depthTest });
        
        if (captured_lines.size() > 1000) {
            captured_lines.erase(captured_lines.begin());
        }
    }

    // ============================================================
    // Internal callback for debug rays (called from hook)
    // ============================================================
    void OnDebugRay(sdk::Vector3 start, sdk::Vector3 dir, float r, float g, float b, float a, float duration, bool depthTest) {
        std::lock_guard<std::mutex> lock(data_mutex);
        stats.rays_captured++;
        captured_rays.push_back({ start, dir, r, g, b, a, duration, depthTest });
        
        if (captured_rays.size() > 1000) {
            captured_rays.erase(captured_rays.begin());
        }
    }

    // ============================================================
    // Internal callback for OnDrawGizmos
    // ============================================================
    void OnOnDrawGizmos(const char* className) {
        std::lock_guard<std::mutex> lock(data_mutex);
        stats.gizmos_called++;
    }

} // namespace debug_draw
} // namespace features
