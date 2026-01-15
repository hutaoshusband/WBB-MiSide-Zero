#include "chams.h"
#include "../sdk/sdk.h"
#include "../config/config.h"

namespace features {
    namespace chams {
        
        bool enabled = false;
        
        // Cache our custom material
        static void* g_ChamsMaterial = nullptr;
        
        void OnTick() {
            // Check config directly or local enabled? features.cpp maps config to enabled boolean pointer usually.
            // But here we might just use the global config.
            if (!config::g_config.visuals.chams.IsActive()) return;

            void* mita = sdk::game::GetMitaManager();
            if (!mita) return;

            // Initialize Material once
            if (!g_ChamsMaterial) {
                // Try standard Unity shaders
                void* shader = sdk::game::FindShader("GUI/Text Shader"); // Always visible usually
                if (!shader) shader = sdk::game::FindShader("Unlit/Color");
                
                if (shader) {
                    g_ChamsMaterial = sdk::game::CreateMaterial(shader);
                    // Set Red Color
                    sdk::game::SetMaterialColor(g_ChamsMaterial, 1.0f, 0.0f, 1.0f, 1.0f); // Magenta/Pink for high visibility
                }
            }

            if (!g_ChamsMaterial) return;

            // Simple validity check for material (if game unloaded it?)
            // We can't easily check if C# object is disposed from raw pointer without more SDK work.
            // But we can ensure we don't apply if we are in a bad state.

            static std::vector<void*> cached_renderers;
            static int timer = 0;

            // Update cache every 100 ticks (approx 1-2 seconds) to avoid spamming recursion
            if (timer <= 0) {
                 cached_renderers = sdk::game::GetRenderers(mita);
                 timer = 100;
            } else {
                timer--;
            }

            if (!g_ChamsMaterial) return;

            for (void* renderer : cached_renderers) {
                if (!renderer) continue;
                sdk::game::SetMaterial(renderer, g_ChamsMaterial);
            }
        }
    }
}
