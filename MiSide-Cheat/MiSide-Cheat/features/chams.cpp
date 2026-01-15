#include "chams.h"
#include "../sdk/sdk.h"
#include "../config/config.h"

namespace features {
    namespace chams {
        
        bool enabled = false;
        
        // Cache our custom material
        static void* g_ChamsMaterial = nullptr;
        static void* cached_renderers[256] = { nullptr }; // Use C-style array instead of vector
        static int cached_renderer_count = 0;
        static int timer = 0;
        
        // Inner function that does the actual work (no C++ objects with destructors)
        static void OnTickInternal(void* mita) {
            // Initialize Material once
            if (!g_ChamsMaterial) {
                // Try standard Unity shaders
                void* shader = sdk::game::FindShader("GUI/Text Shader"); // Always visible usually
                if (!shader) shader = sdk::game::FindShader("Unlit/Color");
                
                if (shader) {
                    g_ChamsMaterial = sdk::game::CreateMaterial(shader);
                    // Set Magenta/Pink for high visibility
                    sdk::game::SetMaterialColor(g_ChamsMaterial, 1.0f, 0.0f, 1.0f, 1.0f);
                }
            }

            if (!g_ChamsMaterial) return;

            // Update cache every 100 ticks (approx 1-2 seconds) to avoid spamming recursion
            if (timer <= 0) {
                std::vector<void*> renderers = sdk::game::GetRenderers(mita);
                cached_renderer_count = 0;
                for (size_t i = 0; i < renderers.size() && i < 256; i++) {
                    cached_renderers[cached_renderer_count++] = renderers[i];
                }
                timer = 100;
            } else {
                timer--;
            }

            for (int i = 0; i < cached_renderer_count; i++) {
                if (!cached_renderers[i]) continue;
                sdk::game::SetMaterial(cached_renderers[i], g_ChamsMaterial);
            }
        }
        
        void OnTick() {
            if (!config::g_config.visuals.chams.IsActive()) return;
            if (!sdk::IsReady()) return;
            
            void* mita = sdk::game::GetMitaManager();
            if (!mita) return;
            
            // SEH protection wrapper (no C++ objects in scope)
            __try {
                OnTickInternal(mita);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                // Reset on crash
                g_ChamsMaterial = nullptr;
                cached_renderer_count = 0;
            }
        }
    }
}


