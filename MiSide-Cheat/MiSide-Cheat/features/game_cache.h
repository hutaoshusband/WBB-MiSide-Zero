#pragma once
#include "../sdk/sdk.h"
#include <mutex>
#include <vector>
#include <string>
#include <atomic>

namespace features {
namespace game_cache {

    // ============================================================
    // Thread-Safe Cached Game Data
    // Updated ONLY from Main Thread (OnTick)
    // Read safely from Render Thread (OnRender)
    // ============================================================

    // Entity data structure for ESP
    struct CachedEntity {
        bool valid = false;
        std::string name;
        sdk::Vector3 worldPos = {0,0,0};
        sdk::Vector3 screenPos = {0,0,0};
        
        // Bone positions (screen space) for accurate bounding box
        sdk::Vector3 headScreen = {0,0,0};
        sdk::Vector3 hipsScreen = {0,0,0};
        sdk::Vector3 leftFootScreen = {0,0,0};
        sdk::Vector3 rightFootScreen = {0,0,0};
        sdk::Vector3 leftHandScreen = {0,0,0};
        sdk::Vector3 rightHandScreen = {0,0,0};
        sdk::Vector3 leftShoulderScreen = {0,0,0};
        sdk::Vector3 rightShoulderScreen = {0,0,0};
        
        int state = 0;
        int moveState = 0;
        bool hasBones = false;
    };

    // Collectible data structure
    struct CachedCollectible {
        bool valid = false;
        std::string name;
        std::string type; // "Card", "Cassette", "Coin", "Item"
        sdk::Vector3 worldPos = {0,0,0};
        sdk::Vector3 screenPos = {0,0,0};
    };

    // Debug info structure
    struct CachedDebugInfo {
        void* playerManager = nullptr;
        void* mitaManager = nullptr;
        void* mainCamera = nullptr;
        void* playerCamera = nullptr;
        void* mitaAnimator = nullptr;
        void* navMeshAgent = nullptr;
        
        sdk::Vector3 playerPos = {0,0,0};
        sdk::Vector3 mitaPos = {0,0,0};
        sdk::Vector3 cameraPos = {0,0,0};
        sdk::Vector3 mitaScreenPos = {0,0,0};
        sdk::Vector3 playerW2S = {0,0,0};
        
        float agentSpeed = 0.0f;
        int activeTweens = 0;
        int pathTweens = 0;
    };

    // Global cached data (thread-safe access)
    struct GameCache {
        std::mutex mutex;
        
        // Mita entity (main target for ESP)
        CachedEntity mita;
        
        // Collectibles list
        std::vector<CachedCollectible> collectibles;
        float collectiblesUpdateTimer = 0.0f;
        
        // Debug info
        CachedDebugInfo debug;
        
        // Frame counter for update throttling
        std::atomic<int> frameCount{0};
    };

    // Global instance
    extern GameCache g_cache;

    // ============================================================
    // Main Thread Functions (call from OnTick only!)
    // ============================================================
    
    // Update all cached data - ONLY call from Main Thread
    void UpdateCache();
    
    // Update Mita entity data
    void UpdateMitaCache();
    
    // Update collectibles cache (throttled to every ~1 second)
    void UpdateCollectiblesCache(float deltaTime);
    
    // Update debug info cache
    void UpdateDebugCache();

    // ============================================================
    // Render Thread Functions (safe to call from OnRender)
    // ============================================================
    
    // Get a copy of Mita data (thread-safe)
    CachedEntity GetMita();
    
    // Get a copy of collectibles list (thread-safe)
    std::vector<CachedCollectible> GetCollectibles();
    
    // Get a copy of debug info (thread-safe)
    CachedDebugInfo GetDebugInfo();

} // namespace game_cache
} // namespace features
