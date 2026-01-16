#include "features.h"
#include "chams.h"
#include "debug_draw.h"
#include "path_prediction.h"
#include "game_cache.h"
#include "esp.h"
#include "debug_view.h"
#include "../config/config.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include "../sdk/sdk.h"

namespace features {

    // ============================================================
    // Get all modules for display in the module list and menu
    // This is where we register all our features
    // ============================================================
    std::vector<ModuleInfo> GetAllModules() {
        std::vector<ModuleInfo> modules;
        
        // Visuals
        modules.push_back({ "ESP",           "Visuals",  &config::g_config.visuals.esp.enabled });
        modules.push_back({ "Chams",         "Visuals",  &config::g_config.visuals.chams.enabled });
        modules.push_back({ "Path Predict",  "Visuals",  &config::g_config.visuals.path_prediction });
        modules.push_back({ "Collectibles",  "Visuals",  &config::g_config.visuals.esp_collectibles });
        
        // Aimbot
        modules.push_back({ "Aimbot",        "Aimbot",   &config::g_config.aimbot.aimbot.enabled });
        
        // Movement
        modules.push_back({ "Speed",         "Movement", &config::g_config.misc.speed_hack.enabled });
        modules.push_back({ "Fly",           "Movement", &config::g_config.misc.fly_hack.enabled });
        modules.push_back({ "NoClip",        "Movement", &config::g_config.misc.no_clip.enabled });
        
        // Game
        modules.push_back({ "Mita Speed",    "Game",     &config::g_config.misc.mita_speed_enabled });
        modules.push_back({ "Debug View",    "Misc",     &config::g_config.misc.debug_view });
        
        return modules;
    }

    std::vector<ModuleInfo> GetEnabledModules() {
        std::vector<ModuleInfo> enabled;
        auto all = GetAllModules();
        
        for (const auto& mod : all) {
            if (mod.IsEnabled()) {
                enabled.push_back(mod);
            }
        }
        
        // Sort by name length (longest first) for pyramid effect in module list
        std::sort(enabled.begin(), enabled.end(), [](const ModuleInfo& a, const ModuleInfo& b) {
            return strlen(a.name) > strlen(b.name);
        });
        
        return enabled;
    }
    
    void Initialize() {
        // Initialize all feature systems here
        // Initialize SDK if not already, to ensure features work without Debug View
        static bool init = false;
        if (!init) {
            init = sdk::Initialize();
        }
        
        // Initialize debug draw system
        debug_draw::Initialize();
    }
    
    void Shutdown() {
        // Cleanup all features
    }
    
    // Called by core when crashes are detected - disables unstable features
    void DisableUnstableFeatures() {
        // Disable features that commonly cause crashes
        // Keep basic features enabled (like menu)
        config::g_config.visuals.esp.enabled = false;
        config::g_config.visuals.chams.enabled = false;
        config::g_config.aimbot.aimbot.enabled = false;
        config::g_config.misc.speed_hack.enabled = false;
        config::g_config.misc.fly_hack.enabled = false;
        config::g_config.misc.no_clip.enabled = false;
        config::g_config.misc.mita_speed_enabled = false;
        config::g_config.misc.debug_draw_hooks = false;
        config::g_config.misc.debug_draw_render = false;
        
        // Disable debug draw hooks if active
        debug_draw::DisableHooks();
    }
    
    void OnTick() {
        // Called every frame from MAIN THREAD - update all enabled features
        // This is the ONLY place where IL2CPP functions should be called!
        
        // Ensure SDK is initialized even if Initialize() wasn't called (e.g. reload)
        static bool sdk_ready = false;
        if (!sdk_ready) {
            sdk_ready = sdk::Initialize();
            if (sdk_ready) {
                // Also attach this thread if not already attached
                sdk::AttachCurrentThread();
            }
        }
        if (!sdk_ready) return;
        
        // Static variables outside SEH block
        static bool lastNoClip = false;
        static float saved_speed = -1.0f;
        static float saved_mita_speed = -1.0f;
        static bool hooksEnabled = false;
        
        // Get game objects
        void* rb = nullptr;
        void* col = nullptr;
        void* move = nullptr;
        void* agent = nullptr;
        void* animator = nullptr;
        
        __try {
            rb = sdk::game::GetPlayerRigidbody();
            col = sdk::game::GetPlayerCollider();
            move = sdk::game::GetPlayerMovement();
            
            // NoClip logic
            bool noClipActive = config::g_config.misc.no_clip.IsActive();
            if (noClipActive != lastNoClip) {
                if (rb && col) {
                    if (noClipActive) {
                        sdk::game::SetRigidbodyKinematic(rb, true);
                        sdk::game::SetColliderEnabled(col, false);
                    } else {
                        sdk::game::SetRigidbodyKinematic(rb, false);
                        sdk::game::SetColliderEnabled(col, true);
                    }
                    lastNoClip = noClipActive;
                }
            }

            // SpeedHack logic
            if (move) {
                if (config::g_config.misc.speed_hack.IsActive()) {
                    // If we haven't saved the speed yet, save it
                    if (saved_speed < 0.0f) {
                        float current = sdk::game::GetSpeed(move);
                        // Only save if it looks like a normal speed (sanity check)
                        if (current > 0.1f && current < 20.0f) {
                             saved_speed = current;
                        } else {
                             // If reading failed or absurd, default to 3.0f (guess)
                             saved_speed = 3.0f;
                        }
                    }
                    
                    // Apply speed hack
                    float target = saved_speed * config::g_config.misc.speed_multiplier;
                    sdk::game::SetSpeed(move, target);
                } else {
                    // If not active, but we have a saved speed, restore it
                    if (saved_speed > 0.0f) {
                        sdk::game::SetSpeed(move, saved_speed);
                        saved_speed = -1.0f; // Reset so we can capture it again next time
                    }
                }
            }

            // Mita Speed Hack Logic
            if (config::g_config.misc.mita_speed_enabled) {
                agent = sdk::game::GetMitaNavMeshAgent();
                animator = sdk::game::GetMitaAnimator();

                if (animator) {
                    sdk::game::SetAnimatorApplyRootMotion(animator, false);
                }

                if (agent) {
                    // Save original speed once
                    if (saved_mita_speed < 0.0f) {
                         float currentSpeed = sdk::game::GetAgentSpeed(agent);
                         if (currentSpeed > 0.1f) saved_mita_speed = currentSpeed;
                         else saved_mita_speed = 3.5f; // Default fallback
                    }

                    // Apply speed
                    sdk::game::SetAgentSpeed(agent, config::g_config.misc.mita_speed);
                    // Also boost acceleration to match speed increase
                    sdk::game::SetAgentAcceleration(agent, 99999.0f);
                }
            } else {
                // Restore logic
                if (saved_mita_speed > 0.0f) {
                     void* anim_restore = sdk::game::GetMitaAnimator();
                     if (anim_restore) sdk::game::SetAnimatorApplyRootMotion(anim_restore, true);

                     void* agent_restore = sdk::game::GetMitaNavMeshAgent();
                     if (agent_restore) {
                         sdk::game::SetAgentSpeed(agent_restore, saved_mita_speed);
                         sdk::game::SetAgentAcceleration(agent_restore, 8.0f); // Reset to something reasonable
                     }
                     saved_mita_speed = -1.0f;
                }
            }

            // Player Modification - FOV Changer
            static float saved_fov = -1.0f;
            if (config::g_config.misc.fov_changer.IsActive()) {
                void* cam = sdk::game::GetPlayerCameraObject();
                if (cam) {
                    // Save original FOV once
                    if (saved_fov < 0.0f) {
                        float current = sdk::game::GetCameraFOV(cam);
                        if (current > 0.1f) saved_fov = current;
                        else saved_fov = 90.0f; // Default fallback
                    }
                    sdk::game::SetCameraFOV(cam, config::g_config.misc.fov_value);
                }
            } else {
                // Restore original FOV
                if (saved_fov > 0.0f) {
                    void* cam = sdk::game::GetPlayerCameraObject();
                    if (cam) {
                        sdk::game::SetCameraFOV(cam, saved_fov);
                    }
                    saved_fov = -1.0f;
                }
            }

            // Chams
            chams::OnTick();
            
            // Debug Draw Hooks Management
            if (config::g_config.misc.debug_draw_hooks != hooksEnabled) {
                if (config::g_config.misc.debug_draw_hooks) {
                    debug_draw::EnableHooks();
                } else {
                    debug_draw::DisableHooks();
                }
                hooksEnabled = config::g_config.misc.debug_draw_hooks;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // IL2CPP call crashed - likely GC issue or invalid pointer
            // Just suppress and continue, crash handler will log it
        }
        
        // ============================================================
        // UPDATE GAME CACHE - Separate try block so it doesn't affect
        // movement features if it crashes
        // ============================================================
        __try {
            game_cache::UpdateCache();
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Cache update failed - ESP/Debug won't update this frame
            // but SpeedHack/NoClip still work
        }
    }

    void OnRender() {
        // Called during render - for overlay drawing
        // CRITICAL: This is the RENDER THREAD
        // DO NOT call any IL2CPP functions here!
        // All data must come from the game_cache module (thread-safe)
        
        // Path Prediction (uses cached data internally)
        RenderPathPrediction();
        
        // ESP Rendering (uses cached data from game_cache)
        esp::RenderAll();
        
        // Debug View Window (uses cached data from game_cache)
        debug_view::Render();
        
        // Debug Draw Rendering - Show captured debug lines and rays
        // Note: This still calls WorldToScreen which is an IL2CPP call
        // TODO: Cache WorldToScreen results in game_cache for full safety
        if (config::g_config.misc.debug_draw_render && debug_draw::IsEnabled()) {
            // Get data outside SEH block
            std::vector<features::debug_draw::DebugLine> lines;
            std::vector<features::debug_draw::DebugRay> rays;
            
            __try {
                lines = debug_draw::GetLines();
                rays = debug_draw::GetRays();
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                // Failed to get debug data, skip rendering
            }
            
            // Rendering outside SEH block
            if (!lines.empty() || !rays.empty()) {
                // Draw captured lines
                int lineCount = 0;
                int maxLines = config::g_config.misc.debug_draw_max_lines;
                
                for (const auto& line : lines) {
                    if (lineCount >= maxLines) break;
                    
                    // Convert world coordinates to screen
                    // NOTE: This is technically an IL2CPP call, but WorldToScreen
                    // has SEH protection internally
                    sdk::Vector3 screenStart = sdk::game::WorldToScreen(line.start);
                    sdk::Vector3 screenEnd = sdk::game::WorldToScreen(line.end);
                    
                    // Only draw if both points are visible
                    if (screenStart.z > 0 && screenEnd.z > 0) {
                        ImColor lineColor(line.r * 255, line.g * 255, line.b * 255, line.a * 255);
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(screenStart.x, screenStart.y),
                            ImVec2(screenEnd.x, screenEnd.y),
                            lineColor,
                            1.0f
                        );
                        lineCount++;
                    }
                }
                
                // Draw captured rays
                int rayCount = 0;
                int maxRays = config::g_config.misc.debug_draw_max_rays;
                
                for (const auto& ray : rays) {
                    if (rayCount >= maxRays) break;
                    
                    // Calculate ray end point (assuming ray is 10 units long)
                    sdk::Vector3 rayEnd = ray.start + (ray.direction * 10.0f);
                    
                    // Convert to screen
                    sdk::Vector3 screenStart = sdk::game::WorldToScreen(ray.start);
                    sdk::Vector3 screenEnd = sdk::game::WorldToScreen(rayEnd);
                    
                    // Only draw if both points are visible
                    if (screenStart.z > 0 && screenEnd.z > 0) {
                        ImColor rayColor(ray.r * 255, ray.g * 255, ray.b * 255, ray.a * 255);
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(screenStart.x, screenStart.y),
                            ImVec2(screenEnd.x, screenEnd.y),
                            rayColor,
                            1.0f
                        );
                        rayCount++;
                    }
                }
            }
        }
    }
}
