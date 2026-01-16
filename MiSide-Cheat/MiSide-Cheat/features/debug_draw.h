#pragma once
#include <vector>
#include <mutex>
#include "../sdk/sdk.h"

namespace features {
namespace debug_draw {

    // ============================================================
    // Debug Line Structure
    // ============================================================
    struct DebugLine {
        sdk::Vector3 start;
        sdk::Vector3 end;
        float r, g, b, a;
        float duration;
        bool depthTest;
    };

    // ============================================================
    // Debug Ray Structure
    // ============================================================
    struct DebugRay {
        sdk::Vector3 start;
        sdk::Vector3 direction;
        float r, g, b, a;
        float duration;
        bool depthTest;
    };

    // ============================================================
    // Stats for Debug Info Display
    // ============================================================
    struct DebugDrawStats {
        int lines_captured = 0;
        int rays_captured = 0;
        int gizmos_called = 0;
        bool hook_active = false;
    };

    // ============================================================
    // Initialize/Shutdown
    // ============================================================
    bool Initialize();
    void Shutdown();

    // ============================================================
    // Hook Management
    // ============================================================
    void EnableHooks();
    void DisableHooks();
    bool IsEnabled();

    // ============================================================
    // Data Access (thread-safe)
    // ============================================================
    std::vector<DebugLine> GetLines();
    std::vector<DebugRay> GetRays();
    DebugDrawStats GetStats();
    void ClearStats();

    // ============================================================
    // Internal Hook Functions
    // ============================================================
    // These will be called by the MinHook hooks
    void OnDebugLine(sdk::Vector3 start, sdk::Vector3 end, float r, float g, float b, float a, float duration, bool depthTest);
    void OnDebugRay(sdk::Vector3 start, sdk::Vector3 dir, float r, float g, float b, float a, float duration, bool depthTest);
    void OnOnDrawGizmos(const char* className);

} // namespace debug_draw
} // namespace features
