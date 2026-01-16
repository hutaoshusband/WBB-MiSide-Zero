#pragma once

namespace features {
namespace debug_view {

    // ============================================================
    // Debug View Rendering
    // All functions are RENDER-THREAD SAFE
    // They only display cached data, no IL2CPP calls!
    // ============================================================

    // Render the debug view window
    void Render();

} // namespace debug_view
} // namespace features
