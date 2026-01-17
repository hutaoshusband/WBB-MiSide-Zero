#include "chams.h"
#include "../sdk/sdk.h"
#include "../config/config.h"
#include <map>
#include <vector>

namespace features {
    namespace chams {
        
        bool enabled = false;
        static void* g_FlatMaterial = nullptr;
        static void* g_GlowMaterial = nullptr;
        static std::map<void*, void*> g_OriginalMaterials;
        static bool g_WasEnabled = false;
        static int g_LastType = -1;
        static int g_RefreshTimer = 0;
        
        static std::map<void*, void*> g_OriginalMaterialsCollectibles;
        static bool g_WasEnabledCollectibles = false;
        static void* g_CollectiblesMaterial = nullptr;

        static std::map<void*, void*> g_OriginalMaterialsWorld;
        static bool g_WasEnabledWorld = false;
        static void* g_WorldMaterial = nullptr;
        static int g_WorldRefreshTimer = 0;

        static void* CreateWallhackMaterial() {
            __try {
                void* shader = sdk::game::FindShader("GUI/Text Shader");
                if (!shader) shader = sdk::game::FindShader("Unlit/Color");
                if (!shader || !sdk::IsValidPtr(shader)) return nullptr;

                void* mat = sdk::game::CreateMaterial(shader);
                if (!mat || !sdk::IsValidPtr(mat)) return nullptr;
                return mat;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                return nullptr;
            }
        }

        static int GetLayer(void* component) {
            __try {
                if (!component) return 0;
                static sdk::Il2CppClass* compClass = nullptr;
                if (!compClass) compClass = sdk::GetClass("UnityEngine", "Component");
                if (!compClass) return 0;

                static sdk::Il2CppMethod* getGO = nullptr;
                if (!getGO) getGO = sdk::GetMethod(compClass, "get_gameObject", 0);
                if (!getGO) return 0;

                void* go = sdk::RuntimeInvoke(getGO, component, nullptr, nullptr);
                if (!go || !sdk::IsValidPtr(go)) return 0;

                static sdk::Il2CppClass* goClass = nullptr;
                if (!goClass) goClass = sdk::GetClass("UnityEngine", "GameObject");
                if (!goClass) return 0;

                static sdk::Il2CppMethod* getLayer = nullptr;
                if (!getLayer) getLayer = sdk::GetMethod(goClass, "get_layer", 0);
                if (!getLayer) return 0;

                void* layerObj = sdk::RuntimeInvoke(getLayer, go, nullptr, nullptr);
                if (!layerObj || !sdk::IsValidPtr(layerObj)) return 0;

                int layerValue = *(int*)((char*)layerObj + 0x10);
                return layerValue;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                return 0;
            }
        }
        
        void OnTick() {
            if (!sdk::IsReady()) return;

            bool currentEnabled = config::g_config.visuals.chams.IsActive();
            int currentType = config::g_config.visuals.chams_type;
            
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

                if (!g_FlatMaterial || !g_GlowMaterial) {
                    return;
                }

                void* mita = sdk::game::GetMitaManager();
                if (mita && sdk::IsValidPtr(mita)) {
                    __try {
                        if (currentType != g_LastType) {
                            g_RefreshTimer = 0;
                            g_LastType = currentType;
                        }

                        void* targetMat = g_FlatMaterial;
                        if (currentType == 2) targetMat = g_GlowMaterial;
                        if (currentType == 1) targetMat = g_FlatMaterial;

                        if (targetMat && sdk::IsValidPtr(targetMat)) {
                            float* col = config::g_config.visuals.chams_color;
                            if (currentType == 2) {
                                float intensity = 2.0f;
                                sdk::game::SetMaterialColor(targetMat, col[0] * intensity, col[1] * intensity, col[2] * intensity, col[3]);
                            } else {
                                sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]);
                            }

                            if (g_RefreshTimer <= 0 || g_OriginalMaterials.empty()) {
                                g_RefreshTimer = 60;
                                std::vector<void*> renderers = sdk::game::GetRenderers(mita);
                                for (void* r : renderers) {
                                    if (!r || !sdk::IsValidPtr(r)) continue;
                                    if (g_OriginalMaterials.find(r) == g_OriginalMaterials.end()) {
                                        void* orig = sdk::game::GetMaterial(r);
                                        if (orig && sdk::IsValidPtr(orig)) g_OriginalMaterials[r] = orig;
                                    }
                                    sdk::game::SetMaterial(r, targetMat);
                                }
                            } else {
                                g_RefreshTimer--;
                                for (auto& pair : g_OriginalMaterials) {
                                    if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(targetMat)) {
                                        sdk::game::SetMaterial(pair.first, targetMat);
                                    }
                                }
                            }
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                    }
                }
            }

            bool colChamsEnabled = config::g_config.visuals.chams_collectibles;

            if (!colChamsEnabled) {
                if (g_WasEnabledCollectibles) {
                    __try {
                        for (auto& pair : g_OriginalMaterialsCollectibles) {
                            if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(pair.second)) {
                                sdk::game::SetMaterial(pair.first, pair.second);
                            }
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                    }
                    g_OriginalMaterialsCollectibles.clear();
                    g_WasEnabledCollectibles = false;
                }
            }
            else {
                g_WasEnabledCollectibles = true;

                if (!g_CollectiblesMaterial) {
                    __try {
                        void* shader = sdk::game::FindShader("GUI/Text Shader");
                        if (shader && sdk::IsValidPtr(shader)) {
                            g_CollectiblesMaterial = sdk::game::CreateMaterial(shader);
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                        g_CollectiblesMaterial = nullptr;
                    }
                }

                if (g_CollectiblesMaterial && sdk::IsValidPtr(g_CollectiblesMaterial)) {
                    __try {
                        float* col = config::g_config.visuals.chams_collectibles_color;
                        sdk::game::SetMaterialColor(g_CollectiblesMaterial, col[0], col[1], col[2], col[3]);

                        static int colRefreshTimer = 0;
                        if (g_OriginalMaterialsCollectibles.empty() || colRefreshTimer <= 0) {
                            colRefreshTimer = 120;

                            std::vector<void*> items = sdk::game::FindObjectsOfTypeAll("ItemPickup");
                            for (void* item : items) {
                                if (!item || !sdk::IsValidPtr(item)) continue;
                                std::vector<void*> renderers = sdk::game::GetRenderers(item);
                                for (void* r : renderers) {
                                    if (!r || !sdk::IsValidPtr(r)) continue;
                                    if (g_OriginalMaterialsCollectibles.find(r) == g_OriginalMaterialsCollectibles.end()) {
                                        void* orig = sdk::game::GetMaterial(r);
                                        if (orig && sdk::IsValidPtr(orig)) g_OriginalMaterialsCollectibles[r] = orig;
                                    }
                                    sdk::game::SetMaterial(r, g_CollectiblesMaterial);
                                }
                            }
                        } else {
                            colRefreshTimer--;
                            for (auto& pair : g_OriginalMaterialsCollectibles) {
                                if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(g_CollectiblesMaterial)) {
                                    sdk::game::SetMaterial(pair.first, g_CollectiblesMaterial);
                                }
                            }
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                    }
                }
            }

            bool worldChamsEnabled = config::g_config.visuals.world_chams;

            if (!worldChamsEnabled) {
                if (g_WasEnabledWorld) {
                    __try {
                        for (auto& pair : g_OriginalMaterialsWorld) {
                            if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(pair.second)) {
                                sdk::game::SetMaterial(pair.first, pair.second);
                            }
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                    }
                    g_OriginalMaterialsWorld.clear();
                    g_WasEnabledWorld = false;
                }
            } else {
                g_WasEnabledWorld = true;

                if (!g_WorldMaterial) {
                    __try {
                        void* shader = sdk::game::FindShader("GUI/Text Shader");
                        if (shader && sdk::IsValidPtr(shader)) {
                            g_WorldMaterial = sdk::game::CreateMaterial(shader);
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                        g_WorldMaterial = nullptr;
                    }
                }

                if (g_WorldMaterial && sdk::IsValidPtr(g_WorldMaterial)) {
                    __try {
                        float* col = config::g_config.visuals.world_chams_color;
                        sdk::game::SetMaterialColor(g_WorldMaterial, col[0], col[1], col[2], col[3]);

                        if (g_OriginalMaterialsWorld.empty() || g_WorldRefreshTimer <= 0) {
                            g_WorldRefreshTimer = 300;

                            std::vector<void*> renderers = sdk::game::FindObjectsOfTypeAll("MeshRenderer");

                            for (void* r : renderers) {
                                if (!r || !sdk::IsValidPtr(r)) continue;

                                int layer = GetLayer(r);
                                if (layer == 5) continue;

                                if (g_OriginalMaterials.find(r) != g_OriginalMaterials.end()) continue;
                                if (g_OriginalMaterialsCollectibles.find(r) != g_OriginalMaterialsCollectibles.end()) continue;

                                if (g_OriginalMaterialsWorld.find(r) == g_OriginalMaterialsWorld.end()) {
                                    void* orig = sdk::game::GetMaterial(r);
                                    if (orig && sdk::IsValidPtr(orig)) g_OriginalMaterialsWorld[r] = orig;
                                }
                                sdk::game::SetMaterial(r, g_WorldMaterial);
                            }
                        } else {
                            g_WorldRefreshTimer--;
                            for (auto& pair : g_OriginalMaterialsWorld) {
                                if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(g_WorldMaterial)) {
                                    sdk::game::SetMaterial(pair.first, g_WorldMaterial);
                                }
                            }
                        }
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                    }
                }
            }
        }
    }
}
