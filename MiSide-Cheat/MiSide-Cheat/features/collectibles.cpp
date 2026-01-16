// collectibles.cpp - DEPRECATED
// This file is kept for backward compatibility but is no longer used.
// Collectibles ESP rendering is now handled by:
// - game_cache.cpp (data caching from main thread)
// - esp.cpp (rendering using cached data)
//
// The new RenderCollectiblesESP function calls the esp module internally.

#include "collectibles.h"
#include "esp.h"

namespace features {

    // Legacy function - now just calls the new esp module
    void RenderCollectiblesESP() {
        // Collectibles ESP is now handled by esp::RenderCollectiblesESP()
        // which uses thread-safe cached data from game_cache
        // This function is kept for compatibility with existing code
        
        // Note: The actual rendering happens through esp::RenderAll() 
        // which is called from features::OnRender()
        // We don't call esp::RenderCollectiblesESP() here to avoid double rendering
    }

}
