#pragma once

namespace features {
namespace esp {

    // ============================================================
    // ESP Rendering Functions
    // All functions here are RENDER-THREAD SAFE
    // They only use cached data, no IL2CPP calls!
    // ============================================================

    // Draw ESP overlay for Mita (main enemy)
    void RenderMitaESP();

    // Draw ESP overlay for collectibles
    void RenderCollectiblesESP();

    // Draw all ESP overlays (convenience function)
    void RenderAll();

} // namespace esp
} // namespace features
