#include "game_cache.h"
#include "../config/config.h"
#include <algorithm>

namespace features {
namespace game_cache {

    // Global cache instance
    GameCache g_cache;

    // ============================================================
    // Main Thread Update Functions
    // ============================================================

    void UpdateMitaCache() {
        CachedEntity newMita;
        
        __try {
            void* mitaMgr = sdk::game::GetMitaManager();
            if (!mitaMgr) {
                std::lock_guard<std::mutex> lock(g_cache.mutex);
                g_cache.mita.valid = false;
                return;
            }

            newMita.valid = true;
            newMita.name = "Mita";
            newMita.worldPos = sdk::game::GetPosition(mitaMgr);
            newMita.screenPos = sdk::game::WorldToScreen(newMita.worldPos);
            newMita.state = sdk::game::GetMitaState();
            newMita.moveState = sdk::game::GetMitaMovementState();

            // Get bone positions if animator available
            void* anim = sdk::game::GetMitaAnimator();
            if (anim) {
                newMita.hasBones = true;
                
                // Get world positions
                sdk::Vector3 headWorld = sdk::game::GetBonePosition(anim, 10);
                sdk::Vector3 hipsWorld = sdk::game::GetBonePosition(anim, 0);
                sdk::Vector3 leftFootWorld = sdk::game::GetBonePosition(anim, 5);
                sdk::Vector3 rightFootWorld = sdk::game::GetBonePosition(anim, 6);
                sdk::Vector3 leftHandWorld = sdk::game::GetBonePosition(anim, 17);
                sdk::Vector3 rightHandWorld = sdk::game::GetBonePosition(anim, 18);
                sdk::Vector3 leftShoulderWorld = sdk::game::GetBonePosition(anim, 11);
                sdk::Vector3 rightShoulderWorld = sdk::game::GetBonePosition(anim, 12);
                
                // Convert all to screen space
                newMita.headScreen = sdk::game::WorldToScreen(headWorld);
                newMita.hipsScreen = sdk::game::WorldToScreen(hipsWorld);
                newMita.leftFootScreen = sdk::game::WorldToScreen(leftFootWorld);
                newMita.rightFootScreen = sdk::game::WorldToScreen(rightFootWorld);
                newMita.leftHandScreen = sdk::game::WorldToScreen(leftHandWorld);
                newMita.rightHandScreen = sdk::game::WorldToScreen(rightHandWorld);
                newMita.leftShoulderScreen = sdk::game::WorldToScreen(leftShoulderWorld);
                newMita.rightShoulderScreen = sdk::game::WorldToScreen(rightShoulderWorld);
            } else {
                newMita.hasBones = false;
                // Fallback: estimate head position
                sdk::Vector3 headWorld = newMita.worldPos;
                headWorld.y += 1.6f;
                newMita.headScreen = sdk::game::WorldToScreen(headWorld);
            }

            // Apply to cache with lock
            {
                std::lock_guard<std::mutex> lock(g_cache.mutex);
                g_cache.mita = newMita;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            std::lock_guard<std::mutex> lock(g_cache.mutex);
            g_cache.mita.valid = false;
        }
    }

    void UpdateCollectiblesCache(float deltaTime) {
        g_cache.collectiblesUpdateTimer += deltaTime;
        
        // Only update every 1 second (performance optimization)
        if (g_cache.collectiblesUpdateTimer < 1.0f && !g_cache.collectibles.empty()) {
            return;
        }
        g_cache.collectiblesUpdateTimer = 0.0f;

        std::vector<CachedCollectible> newCollectibles;

        __try {
            std::vector<void*> objects = sdk::game::FindObjectsOfTypeAll("ItemPickup");
            
            for (void* obj : objects) {
                if (!sdk::IsValidPtr(obj)) continue;

                CachedCollectible col;
                col.valid = true;
                
                const char* name = sdk::game::GetObjectName(obj);
                if (!name) continue;
                
                col.name = name;
                col.worldPos = sdk::game::GetPosition(obj);
                col.screenPos = sdk::game::WorldToScreen(col.worldPos);
                
                // Determine type
                std::string nameLower = col.name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                
                if (nameLower.find("card") != std::string::npos) {
                    col.type = "Card";
                } else if (nameLower.find("cassette") != std::string::npos || 
                           nameLower.find("tape") != std::string::npos ||
                           nameLower.find("cartridge") != std::string::npos) {
                    col.type = "Cassette";
                } else if (nameLower.find("coin") != std::string::npos ||
                           nameLower.find("money") != std::string::npos) {
                    col.type = "Coin";
                } else {
                    col.type = "Item";
                }
                
                newCollectibles.push_back(col);
            }
            
            // Apply to cache with lock
            {
                std::lock_guard<std::mutex> lock(g_cache.mutex);
                g_cache.collectibles = std::move(newCollectibles);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Keep old cache on failure
        }
    }

    void UpdateDebugCache() {
        CachedDebugInfo newDebug;
        
        __try {
            newDebug.playerManager = sdk::game::GetPlayerManager();
            newDebug.mitaManager = sdk::game::GetMitaManager();
            newDebug.mainCamera = sdk::game::GetMainCamera();
            newDebug.playerCamera = sdk::game::GetPlayerCamera();
            
            if (newDebug.playerManager) {
                newDebug.playerPos = sdk::game::GetPosition(newDebug.playerManager);
            }
            
            if (newDebug.mitaManager) {
                newDebug.mitaPos = sdk::game::GetPosition(newDebug.mitaManager);
                newDebug.mitaScreenPos = sdk::game::WorldToScreen(newDebug.mitaPos);
                newDebug.mitaAnimator = sdk::game::GetMitaAnimator();
                newDebug.navMeshAgent = sdk::game::GetMitaNavMeshAgent();
                
                if (newDebug.navMeshAgent) {
                    newDebug.agentSpeed = sdk::game::GetAgentSpeed(newDebug.navMeshAgent);
                }
            }
            
            void* activeCam = newDebug.mainCamera ? newDebug.mainCamera : newDebug.playerCamera;
            if (activeCam) {
                newDebug.cameraPos = sdk::game::GetPosition(activeCam);
            }
            
            if (newDebug.playerManager && activeCam) {
                newDebug.playerW2S = sdk::game::WorldToScreen(newDebug.playerPos);
            }
            
            // DOTween stats (expensive, update less frequently)
            static int tweenUpdateCounter = 0;
            if (++tweenUpdateCounter >= 30) {
                tweenUpdateCounter = 0;
                auto tweens = sdk::game::GetActiveTweens();
                newDebug.activeTweens = (int)tweens.size();
                newDebug.pathTweens = 0;
                for (auto t : tweens) {
                    if (sdk::game::GetTweenPathPoints(t).size() > 1) {
                        newDebug.pathTweens++;
                    }
                }
            } else {
                // Keep previous values
                std::lock_guard<std::mutex> lock(g_cache.mutex);
                newDebug.activeTweens = g_cache.debug.activeTweens;
                newDebug.pathTweens = g_cache.debug.pathTweens;
            }
            
            // Apply with lock
            {
                std::lock_guard<std::mutex> lock(g_cache.mutex);
                g_cache.debug = newDebug;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Keep old debug cache on failure
        }
    }

    void UpdateCache() {
        // Only update if ESP or Debug View is active
        if (config::g_config.visuals.esp.IsActive()) {
            UpdateMitaCache();
        }
        
        if (config::g_config.visuals.esp_collectibles) {
            UpdateCollectiblesCache(0.016f); // ~60fps delta
        }
        
        if (config::g_config.misc.debug_view) {
            UpdateDebugCache();
        }
        
        g_cache.frameCount++;
    }

    // ============================================================
    // Render Thread Safe Getters (return copies)
    // ============================================================

    CachedEntity GetMita() {
        std::lock_guard<std::mutex> lock(g_cache.mutex);
        return g_cache.mita;
    }

    std::vector<CachedCollectible> GetCollectibles() {
        std::lock_guard<std::mutex> lock(g_cache.mutex);
        return g_cache.collectibles;
    }

    CachedDebugInfo GetDebugInfo() {
        std::lock_guard<std::mutex> lock(g_cache.mutex);
        return g_cache.debug;
    }

} // namespace game_cache
} // namespace features
