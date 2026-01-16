#include "chams.h"
#include "../sdk/sdk.h"
#include "../config/config.h"
#include <map>

namespace features {
    namespace chams {
        
        bool enabled = false;
        
        // Materials
        static void* g_FlatMaterial = nullptr;
        static void* g_GlowMaterial = nullptr;
        
        // Restore map: Renderer -> Original Material
        static std::map<void*, void*> g_OriginalMaterials;
        static bool g_WasEnabled = false;
        static int g_LastType = -1;
        
        // Cache timer
        static int g_RefreshTimer = 0;
        
        // Helper to create a wallhack material
        static void* CreateWallhackMaterial() {
            void* shader = sdk::game::FindShader("GUI/Text Shader");
            if (!shader) shader = sdk::game::FindShader("Unlit/Color");
            if (!shader) return nullptr;
            
            void* mat = sdk::game::CreateMaterial(shader);
            return mat;
        }
        
        void OnTick() {
            bool currentEnabled = config::g_config.visuals.chams.IsActive();
            int currentType = config::g_config.visuals.chams_type;
            
            // Handle Disable / Restore
            if (!currentEnabled) {
                if (g_WasEnabled) {
                    // Restore original materials
                    for (auto& pair : g_OriginalMaterials) {
                        if (sdk::IsValidPtr(pair.first) && sdk::IsValidPtr(pair.second)) {
                            sdk::game::SetMaterial(pair.first, pair.second);
                        }
                    }
                    g_OriginalMaterials.clear();
                    g_WasEnabled = false;
                    g_RefreshTimer = 0;
                }
                return;
            }
            
            if (!sdk::IsReady()) return;
            
            void* mita = sdk::game::GetMitaManager();
            if (!mita) return;
            
            // Check if type changed - force refresh
            if (currentType != g_LastType) {
                g_RefreshTimer = 0;
                g_LastType = currentType;
            }
            
            g_WasEnabled = true;
            
            // Create materials
            if (!g_FlatMaterial) {
                g_FlatMaterial = CreateWallhackMaterial();
            }
            
            if (!g_GlowMaterial) {
                g_GlowMaterial = CreateWallhackMaterial();
            }
            
            // Select material based on type
            void* targetMat = g_FlatMaterial;
            if (currentType == 2) targetMat = g_GlowMaterial;
            if (currentType == 1) targetMat = g_FlatMaterial; // Textured fallback

            if (!targetMat) return;
            
            // Update material colors
            float* col = config::g_config.visuals.chams_color;
            
            if (currentType == 2) {
                // Glow: Brighter color
                float intensity = 2.0f;
                sdk::game::SetMaterialColor(targetMat, 
                    col[0] * intensity, 
                    col[1] * intensity, 
                    col[2] * intensity, 
                    col[3]);
            } else {
                // Flat: Normal color
                sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]);
            }
            
            // Refresh renderers periodically OR if cache is empty
            if (g_RefreshTimer <= 0 || g_OriginalMaterials.empty()) {
                g_RefreshTimer = 60; // ~1s at 60fps
                
                std::vector<void*> renderers = sdk::game::GetRenderers(mita);
                for (void* r : renderers) {
                    if (!r) continue;
                    
                    // Save original material if new
                    if (g_OriginalMaterials.find(r) == g_OriginalMaterials.end()) {
                        void* orig = sdk::game::GetMaterial(r);
                        if (orig) {
                            g_OriginalMaterials[r] = orig;
                        }
                    }
                    
                    // Apply chams
                    sdk::game::SetMaterial(r, targetMat);
                }
            } else {
                g_RefreshTimer--;
                
                // Apply to all cached renderers
                for (auto& pair : g_OriginalMaterials) {
                    if (sdk::IsValidPtr(pair.first)) {
                        sdk::game::SetMaterial(pair.first, targetMat);
                    }
                }
            }
        }
    }
}
