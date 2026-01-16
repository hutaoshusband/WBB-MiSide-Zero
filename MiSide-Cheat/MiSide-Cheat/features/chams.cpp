#include "chams.h"
#include "../sdk/sdk.h"
#include "../config/config.h"
#include <map>
#include <vector>

namespace features {
    namespace chams {
        
        // Mita Chams State
        bool enabled = false;
        static void* g_FlatMaterial = nullptr;
        static void* g_GlowMaterial = nullptr;
        static std::map<void*, void*> g_OriginalMaterials;
        static bool g_WasEnabled = false;
        static int g_LastType = -1;
        static int g_RefreshTimer = 0;
        
        // Collectibles Chams State
        static std::map<void*, void*> g_OriginalMaterialsCollectibles;
        static bool g_WasEnabledCollectibles = false;
        static void* g_CollectiblesMaterial = nullptr;

        // World Chams State
        static std::map<void*, void*> g_OriginalMaterialsWorld;
        static bool g_WasEnabledWorld = false;
        static void* g_WorldMaterial = nullptr;
        static int g_WorldRefreshTimer = 0;

        // Helper to create a wallhack material
        static void* CreateWallhackMaterial() {
            void* shader = sdk::game::FindShader("GUI/Text Shader");
            if (!shader) shader = sdk::game::FindShader("Unlit/Color");
            if (!shader) return nullptr;
            
            void* mat = sdk::game::CreateMaterial(shader);
            return mat;
        }

        // Helper to get layer
        static int GetLayer(void* component) {
            if (!component) return 0;
            static sdk::Il2CppClass* compClass = nullptr;
            if (!compClass) compClass = sdk::GetClass("UnityEngine", "Component");
            
            static sdk::Il2CppMethod* getGO = nullptr;
            if (!getGO) getGO = sdk::GetMethod(compClass, "get_gameObject", 0);
            
            void* go = sdk::RuntimeInvoke(getGO, component, nullptr, nullptr);
            if (!go) return 0;
            
            static sdk::Il2CppClass* goClass = nullptr;
            if (!goClass) goClass = sdk::GetClass("UnityEngine", "GameObject");
            
            static sdk::Il2CppMethod* getLayer = nullptr;
            if (!getLayer) getLayer = sdk::GetMethod(goClass, "get_layer", 0);
            
            void* layerObj = sdk::RuntimeInvoke(getLayer, go, nullptr, nullptr);
            if (layerObj) return *(int*)((char*)layerObj + 0x10);
            return 0;
        }
        
        void OnTick() {
            if (!sdk::IsReady()) return;

            // ===================================
            // Mita / Entity Chams Logic
            // ===================================
            bool currentEnabled = config::g_config.visuals.chams.IsActive();
            int currentType = config::g_config.visuals.chams_type;
            
            // Handle Disable / Restore Mita
            if (!currentEnabled) {
                if (g_WasEnabled) {
                    for (auto& pair : g_OriginalMaterials) {
                        if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(pair.second)) {
                            sdk::game::SetMaterial(pair.first, pair.second);
                        }
                    }
                    g_OriginalMaterials.clear();
                    g_WasEnabled = false;
                    g_RefreshTimer = 0;
                }
            } else {
                g_WasEnabled = true;
                
                if (!g_FlatMaterial) g_FlatMaterial = CreateWallhackMaterial();
                if (!g_GlowMaterial) g_GlowMaterial = CreateWallhackMaterial();
                
                void* mita = sdk::game::GetMitaManager();
                if (mita) {
                    if (currentType != g_LastType) {
                        g_RefreshTimer = 0;
                        g_LastType = currentType;
                    }

                    void* targetMat = g_FlatMaterial;
                    if (currentType == 2) targetMat = g_GlowMaterial;
                    if (currentType == 1) targetMat = g_FlatMaterial; 
                    
                    if (targetMat) {
                        float* col = config::g_config.visuals.chams_color;
                        if (currentType == 2) {
                            float intensity = 2.0f; 
                            sdk::game::SetMaterialColor(targetMat, col[0] * intensity, col[1] * intensity, col[2] * intensity, col[3]);
                        } else {
                            sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]);
                        }
                        
                        // Scan & Apply
                        if (g_RefreshTimer <= 0 || g_OriginalMaterials.empty()) {
                            g_RefreshTimer = 60; // 1s
                            std::vector<void*> renderers = sdk::game::GetRenderers(mita);
                            for (void* r : renderers) {
                                if (!r) continue;
                                if (g_OriginalMaterials.find(r) == g_OriginalMaterials.end()) {
                                    void* orig = sdk::game::GetMaterial(r);
                                    if (orig) g_OriginalMaterials[r] = orig;
                                }
                                sdk::game::SetMaterial(r, targetMat);
                            }
                        } else {
                            g_RefreshTimer--;
                            for (auto& pair : g_OriginalMaterials) {
                                if (sdk::IsValidPtr(pair.first)) sdk::game::SetMaterial(pair.first, targetMat);
                            }
                        }
                    }
                }
            }

            // ===================================
            // Collectibles Chams Logic (Flat)
            // ===================================
            bool colChamsEnabled = config::g_config.visuals.chams_collectibles;
            
            if (!colChamsEnabled) {
                if (g_WasEnabledCollectibles) {
                    for (auto& pair : g_OriginalMaterialsCollectibles) {
                        if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(pair.second)) {
                            sdk::game::SetMaterial(pair.first, pair.second);
                        }
                    }
                    g_OriginalMaterialsCollectibles.clear();
                    g_WasEnabledCollectibles = false;
                }
            }
            else {
                g_WasEnabledCollectibles = true;
                
                if (!g_CollectiblesMaterial) {
                     void* shader = sdk::game::FindShader("GUI/Text Shader"); // Flat
                     if (shader) g_CollectiblesMaterial = sdk::game::CreateMaterial(shader);
                }
                
                if (g_CollectiblesMaterial) {
                    float* col = config::g_config.visuals.chams_collectibles_color;
                    sdk::game::SetMaterialColor(g_CollectiblesMaterial, col[0], col[1], col[2], col[3]);
                    
                    static int colRefreshTimer = 0;
                    if (g_OriginalMaterialsCollectibles.empty() || colRefreshTimer <= 0) {
                        colRefreshTimer = 120; // Re-scan every 2s
                        
                        std::vector<void*> items = sdk::game::FindObjectsOfTypeAll("ItemPickup");
                        for (void* item : items) {
                            if (!item) continue;
                            std::vector<void*> renderers = sdk::game::GetRenderers(item);
                            for (void* r : renderers) {
                                if (!r) continue;
                                if (g_OriginalMaterialsCollectibles.find(r) == g_OriginalMaterialsCollectibles.end()) {
                                    void* orig = sdk::game::GetMaterial(r);
                                    if (orig) g_OriginalMaterialsCollectibles[r] = orig;
                                }
                                sdk::game::SetMaterial(r, g_CollectiblesMaterial);
                            }
                        }
                    } else {
                         colRefreshTimer--;
                         // Re-apply to known
                         for (auto& pair : g_OriginalMaterialsCollectibles) {
                            if (sdk::IsValidPtr(pair.first)) sdk::game::SetMaterial(pair.first, g_CollectiblesMaterial);
                        }
                    }
                }
            }

            // ===================================
            // World Chams Logic
            // ===================================
            bool worldChamsEnabled = config::g_config.visuals.world_chams;

            if (!worldChamsEnabled) {
                if (g_WasEnabledWorld) {
                    for (auto& pair : g_OriginalMaterialsWorld) {
                        if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(pair.second)) {
                            sdk::game::SetMaterial(pair.first, pair.second);
                        }
                    }
                    g_OriginalMaterialsWorld.clear();
                    g_WasEnabledWorld = false;
                }
            } else {
                g_WasEnabledWorld = true;

                if (!g_WorldMaterial) {
                     void* shader = sdk::game::FindShader("GUI/Text Shader"); // Flat
                     if (shader) g_WorldMaterial = sdk::game::CreateMaterial(shader);
                }

                if (g_WorldMaterial) {
                    float* col = config::g_config.visuals.world_chams_color;
                    sdk::game::SetMaterialColor(g_WorldMaterial, col[0], col[1], col[2], col[3]);

                    if (g_OriginalMaterialsWorld.empty() || g_WorldRefreshTimer <= 0) {
                        g_WorldRefreshTimer = 300; // Rescan every 5s

                        std::vector<void*> renderers = sdk::game::FindObjectsOfTypeAll("MeshRenderer");
                        
                        for (void* r : renderers) {
                            if (!r) continue;
                            
                            // Exclude UI (Layer 5)
                            int layer = GetLayer(r);
                            if (layer == 5) continue;
                            
                            // Exclude already handled (Mita/Collectibles)
                            if (g_OriginalMaterials.find(r) != g_OriginalMaterials.end()) continue;
                            if (g_OriginalMaterialsCollectibles.find(r) != g_OriginalMaterialsCollectibles.end()) continue;

                            if (g_OriginalMaterialsWorld.find(r) == g_OriginalMaterialsWorld.end()) {
                                void* orig = sdk::game::GetMaterial(r);
                                if (orig) g_OriginalMaterialsWorld[r] = orig;
                            }
                            sdk::game::SetMaterial(r, g_WorldMaterial);
                        }
                    } else {
                        g_WorldRefreshTimer--;
                        // Re-apply to known
                        for (auto& pair : g_OriginalMaterialsWorld) {
                            if (sdk::IsValidPtr(pair.first)) sdk::game::SetMaterial(pair.first, g_WorldMaterial);
                        }
                    }
                }
            }
        }
    }
}
