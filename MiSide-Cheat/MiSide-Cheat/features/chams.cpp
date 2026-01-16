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
        
        // Partial body modulation materials
        static void* g_HeadMaterial = nullptr;
        static void* g_BodyMaterial = nullptr;
        static void* g_LegsMaterial = nullptr;
        static void* g_ArmsMaterial = nullptr;
        
        // Restore map: Renderer -> Original Material
        static std::map<void*, void*> g_OriginalMaterials;
        static bool g_WasEnabled = false;
        static int g_LastType = -1;
        static bool g_LastPartialBody = false;
        
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
            bool partialBodyEnabled = config::g_config.visuals.chams_partial_body;
            
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
                    // Reset refresh timer so next enable will refresh immediately
                    g_RefreshTimer = 0;
                }
                return;
            }
            
            if (!sdk::IsReady()) return;
            
            void* mita = sdk::game::GetMitaManager();
            if (!mita) return;
            
            // Check if settings changed - force refresh
            if (partialBodyEnabled != g_LastPartialBody || currentType != g_LastType) {
                g_RefreshTimer = 0; // Force immediate refresh
                g_LastPartialBody = partialBodyEnabled;
                g_LastType = currentType;
            }
            
            // Logic start
            g_WasEnabled = true;
            
            // Create base materials
            if (!g_FlatMaterial) {
                g_FlatMaterial = CreateWallhackMaterial();
            }
            
            if (!g_GlowMaterial) {
                g_GlowMaterial = CreateWallhackMaterial();
            }
            
            // Select material based on type
            // Note: Both Flat and Glow use the same shader (GUI/Text Shader) that draws through walls
            // The difference is in the color intensity - Glow uses brighter/HDR-like colors
            void* targetMat = g_FlatMaterial;
            if (currentType == 2) targetMat = g_GlowMaterial;
            if (currentType == 1) targetMat = g_FlatMaterial; // Textured fallback to flat

            if (!targetMat) return;
            
            // Update material colors
            float* col = config::g_config.visuals.chams_color;
            
            if (currentType == 2) {
                // Glow: Use brighter color (simulated HDR by multiplying)
                float intensity = 2.0f; // Make it glow brighter
                sdk::game::SetMaterialColor(targetMat, 
                    col[0] * intensity, 
                    col[1] * intensity, 
                    col[2] * intensity, 
                    col[3]);
            } else {
                // Flat: Normal color
                sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]);
            }
            
            // Handle partial body materials
            if (partialBodyEnabled) {
                // Create materials for each body part if needed
                if (!g_HeadMaterial) g_HeadMaterial = CreateWallhackMaterial();
                if (!g_BodyMaterial) g_BodyMaterial = CreateWallhackMaterial();
                if (!g_LegsMaterial) g_LegsMaterial = CreateWallhackMaterial();
                if (!g_ArmsMaterial) g_ArmsMaterial = CreateWallhackMaterial();
                
                // Apply colors to each body part material
                float* headCol = config::g_config.visuals.chams_head_color;
                float* bodyCol = config::g_config.visuals.chams_body_color;
                float* legsCol = config::g_config.visuals.chams_legs_color;
                float* armsCol = config::g_config.visuals.chams_arms_color;
                
                float intensity = (currentType == 2) ? 2.0f : 1.0f;
                
                if (g_HeadMaterial) sdk::game::SetMaterialColor(g_HeadMaterial, headCol[0]*intensity, headCol[1]*intensity, headCol[2]*intensity, headCol[3]);
                if (g_BodyMaterial) sdk::game::SetMaterialColor(g_BodyMaterial, bodyCol[0]*intensity, bodyCol[1]*intensity, bodyCol[2]*intensity, bodyCol[3]);
                if (g_LegsMaterial) sdk::game::SetMaterialColor(g_LegsMaterial, legsCol[0]*intensity, legsCol[1]*intensity, legsCol[2]*intensity, legsCol[3]);
                if (g_ArmsMaterial) sdk::game::SetMaterialColor(g_ArmsMaterial, armsCol[0]*intensity, armsCol[1]*intensity, armsCol[2]*intensity, armsCol[3]);
            }
            
            // Helper lambda to apply material to a renderer
            auto ApplyMaterial = [&](void* renderer) {
                if (!renderer) return;
                
                if (partialBodyEnabled) {
                    // Get body part and apply appropriate material
                    sdk::game::BodyPart part = sdk::game::GetRendererBodyPart(renderer);
                    void* matToApply = targetMat; // Default fallback
                    
                    switch (part) {
                        case sdk::game::BodyPart_Head:
                            if (config::g_config.visuals.chams_head && g_HeadMaterial) {
                                matToApply = g_HeadMaterial;
                            }
                            break;
                        case sdk::game::BodyPart_Body:
                            if (config::g_config.visuals.chams_body && g_BodyMaterial) {
                                matToApply = g_BodyMaterial;
                            }
                            break;
                        case sdk::game::BodyPart_Legs:
                            if (config::g_config.visuals.chams_legs && g_LegsMaterial) {
                                matToApply = g_LegsMaterial;
                            }
                            break;
                        case sdk::game::BodyPart_Arms:
                            if (config::g_config.visuals.chams_arms && g_ArmsMaterial) {
                                matToApply = g_ArmsMaterial;
                            }
                            break;
                        default:
                            // Unknown body part - still apply targetMat so it shows
                            matToApply = targetMat;
                            break;
                    }
                    
                    if (matToApply) {
                        sdk::game::SetMaterial(renderer, matToApply);
                    }
                } else {
                    // Standard chams - apply same material to all renderers
                    sdk::game::SetMaterial(renderer, targetMat);
                }
            };
            
            // Refresh renderers periodically OR if cache is empty
            if (g_RefreshTimer <= 0 || g_OriginalMaterials.empty()) {
                g_RefreshTimer = 60; // 1s
                
                std::vector<void*> renderers = sdk::game::GetRenderers(mita);
                for (void* r : renderers) {
                    if (!r) continue;
                    
                    // If new renderer, save original
                    if (g_OriginalMaterials.find(r) == g_OriginalMaterials.end()) {
                        void* orig = sdk::game::GetMaterial(r);
                        if (orig) {
                            g_OriginalMaterials[r] = orig;
                        }
                    }
                    
                    // Apply chams
                    ApplyMaterial(r);
                }
            } else {
                g_RefreshTimer--;
                
                // Apply to all cached renderers
                for (auto& pair : g_OriginalMaterials) {
                    if (sdk::IsValidPtr(pair.first)) {
                        ApplyMaterial(pair.first);
                    }
                }
            }
        }
    }
}
