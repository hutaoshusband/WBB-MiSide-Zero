#include "features.h"
#include "chams.h"
#include "debug_draw.h"
#include "path_prediction.h"
#include "collectibles.h"
#include "../config/config.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include "../sdk/sdk.h"

namespace features {

// Helper function to expand bounds (outside OnRender to avoid SEH issues)
static void ExpandBounds(const sdk::Vector3& p, float& minX, float& maxX, float& minY, float& maxY) {
    if (p.z > 0) { // Only if visible
        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }
}

// ... (existing code helpers)

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
        // Called every frame - update all enabled features
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

            // Jump Power - Game uses kiriMoveBasic which doesn't have jumping
            // This feature is disabled in UI

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

        // World Features
        // config::g_config.visuals.fullbright.IsActive() ...
    }

    void OnRender() {
        // Called during render - for overlay drawing
        
        // CRITICAL: Do NOT attach this thread to IL2CPP!
        // The render thread should NOT be attached to IL2CPP runtime.
        // IL2CPP operations should only happen from the main thread (OnTick).
        // The main thread is already attached in core::MainThread().
        // Attaching from render thread causes "Fatal error in GC: Collecting from unknown thread"

        // Path Prediction
        RenderPathPrediction();
        RenderCollectiblesESP();
        
        if (config::g_config.misc.debug_view) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
            ImGui::Begin("Debug View (MiSide-Zero)", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
            {
                ImGui::TextColored(ImVec4(0.58f, 0.72f, 0.02f, 1.0f), "WallbangBros Game Analysis");
                ImGui::Separator();
                
                // Initialize SDK if not already
                static bool sdkInit = false;
                if (!sdkInit) {
                    sdkInit = sdk::Initialize();
                }
                
                // Get statistics outside SEH block
                debug_draw::DebugDrawStats debugStats{};
                
                __try {
                    if (!sdk::IsReady()) {
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "SDK Not Ready");
                    } else {
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "SDK Initialized");
                        
                        ImGui::TextWrapped("SDK Log: %s", sdk::GetLastLog());
                        ImGui::Separator();

                        // 1. Local Player Check
                        void* pm = sdk::game::GetPlayerManager();
                        void* mitaMgr = sdk::game::GetMitaManager();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Local Player]");
                        ImGui::Text("PlayerManager: %p", pm);
                        
                        if (pm) {
                            sdk::Vector3 pos = sdk::game::GetPosition(pm); 
                            ImGui::BulletText("Pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                        }
                        
                        ImGui::Spacing();
                        
                        // 2. Camera Check (Dual Check)
                        void* mainCam = sdk::game::GetMainCamera();
                        void* playerCam = sdk::game::GetPlayerCamera();
                        
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Camera System]");
                        ImGui::Text("Camera.main: %p", mainCam);
                        ImGui::Text("PlayerManager.playerCam: %p", playerCam);
                        
                        void* activeCam = mainCam ? mainCam : playerCam;
                        
                        if (mainCam) {
                            sdk::Vector3 mPos = sdk::game::GetPosition(mainCam);
                            ImGui::Text("MainCam Pos: %.2f, %.2f, %.2f", mPos.x, mPos.y, mPos.z);
                        }
                        if (playerCam) {
                            sdk::Vector3 pPos = sdk::game::GetPosition(playerCam);
                            ImGui::Text("PlayerCam Pos: %.2f, %.2f, %.2f", pPos.x, pPos.y, pPos.z);
                        }
                        
                        if (activeCam) {
                            sdk::Vector3 camPos = sdk::game::GetPosition(activeCam);
                            ImGui::BulletText("Active Cam Pos: %.2f, %.2f, %.2f", camPos.x, camPos.y, camPos.z);
                        } else {
                            ImGui::TextColored(ImVec4(1, 0, 0, 1), "NO CAMERA FOUND");
                        }
                        
                        ImGui::Spacing();

                        // 3. Mita Check
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Mita]");
                        ImGui::Text("MitaManager: %p", mitaMgr);
                        void* mitaAnim = sdk::game::GetMitaAnimator();
                        ImGui::Text("MitaAnimator: %p", mitaAnim);
                        
                        if (mitaMgr) {
                            sdk::Vector3 mitaPos = sdk::game::GetPosition(mitaMgr);
                            ImGui::BulletText("Pos: %.2f, %.2f, %.2f", mitaPos.x, mitaPos.y, mitaPos.z);
                            
                            // W2S Test on Mita
                            sdk::Vector3 screenMita = sdk::game::WorldToScreen(mitaPos);
                            ImGui::Text("Screen: %.1f, %.1f (Z: %.1f)", screenMita.x, screenMita.y, screenMita.z);

                            // NavMeshAgent Info
                            void* agent = sdk::game::GetMitaNavMeshAgent();
                            ImGui::Text("NavMeshAgent: %p", agent);
                            if (agent) {
                                float speed = sdk::game::GetAgentSpeed(agent);
                                ImGui::Text("Agent Speed: %.2f", speed);
                            }
                        }

                        // World To Screen Test
                        if (pm && activeCam) {
                             sdk::Vector3 worldPos = sdk::game::GetPosition(pm);
                             sdk::Vector3 screenPos = sdk::game::WorldToScreen(worldPos);
                             
                             ImGui::Text("Player W2S: %.1f, %.1f", screenPos.x, screenPos.y);
                        }
                        
                        // Debug Draw Statistics
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Debug Draw Hooks]");
                        debugStats = debug_draw::GetStats();
                        ImGui::Text("Hooks Active: %s", debugStats.hook_active ? "YES" : "NO");
                        ImGui::Text("Lines Captured: %d", debugStats.lines_captured);
                        ImGui::Text("Rays Captured: %d", debugStats.rays_captured);
                        ImGui::Text("Gizmos Called: %d", debugStats.gizmos_called);
                        
                        // Current Settings
                        ImGui::Text("Render Debug: %s", config::g_config.misc.debug_draw_render ? "YES" : "NO");
                        ImGui::Text("Max Lines: %d", config::g_config.misc.debug_draw_max_lines);
                        ImGui::Text("Max Rays: %d", config::g_config.misc.debug_draw_max_rays);

                        // DOTween Stats
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[DOTween]");
                        static int activeTweensCount = 0;
                        static int pathTweensCount = 0;
                        if (ImGui::GetFrameCount() % 30 == 0) { // Update every 30 frames
                             auto tweens = sdk::game::GetActiveTweens();
                             activeTweensCount = (int)tweens.size();
                             pathTweensCount = 0;
                             for(auto t : tweens) {
                                 if (sdk::game::GetTweenPathPoints(t).size() > 1) pathTweensCount++;
                             }
                        }
                        ImGui::Text("Active Tweens: %d", activeTweensCount);
                        ImGui::Text("Path Tweens: %d", pathTweensCount);
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "IL2CPP Exception in Debug View!");
                }
                
                ImGui::Separator();
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                
                if (ImGui::Button("Close Debug View")) {
                    config::g_config.misc.debug_view = false;
                }
            }
            ImGui::End();
        }


            // Debug Draw Rendering - Show captured debug lines and rays
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

            // ESP Rendering with SEH Protection
            if (config::g_config.visuals.esp.IsActive()) {
                void* mita = nullptr;
                void* anim = nullptr;
                
                __try {
                    mita = sdk::game::GetMitaManager();
                    
                    if (mita) {
                        anim = sdk::game::GetMitaAnimator();
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    // Silently ignore ESP crashes - will retry next frame
                    return;
                }
                
                if (mita) {
                    if (anim) {
                        // Get key bone positions for bounding box
                        sdk::Vector3 headWorld = sdk::game::GetBonePosition(anim, 10);      // Head
                        sdk::Vector3 hipsWorld = sdk::game::GetBonePosition(anim, 0);       // Hips (center mass)
                        sdk::Vector3 leftFootWorld = sdk::game::GetBonePosition(anim, 5);   // Left Foot
                        sdk::Vector3 rightFootWorld = sdk::game::GetBonePosition(anim, 6);  // Right Foot
                        sdk::Vector3 leftHandWorld = sdk::game::GetBonePosition(anim, 17);  // Left Hand
                        sdk::Vector3 rightHandWorld = sdk::game::GetBonePosition(anim, 18); // Right Hand
                        sdk::Vector3 leftShoulderWorld = sdk::game::GetBonePosition(anim, 11);  // Left Shoulder
                        sdk::Vector3 rightShoulderWorld = sdk::game::GetBonePosition(anim, 12); // Right Shoulder
                        
                        // Convert to screen space
                        sdk::Vector3 headScreen = sdk::game::WorldToScreen(headWorld);
                        sdk::Vector3 hipsScreen = sdk::game::WorldToScreen(hipsWorld);
                        sdk::Vector3 leftFootScreen = sdk::game::WorldToScreen(leftFootWorld);
                        sdk::Vector3 rightFootScreen = sdk::game::WorldToScreen(rightFootWorld);
                        sdk::Vector3 leftHandScreen = sdk::game::WorldToScreen(leftHandWorld);
                        sdk::Vector3 rightHandScreen = sdk::game::WorldToScreen(rightHandWorld);
                        sdk::Vector3 leftShoulderScreen = sdk::game::WorldToScreen(leftShoulderWorld);
                        sdk::Vector3 rightShoulderScreen = sdk::game::WorldToScreen(rightShoulderWorld);
                        
                        // Check if at least head and feet are visible (basic visibility check)
                        if (headScreen.z > 0 && (leftFootScreen.z > 0 || rightFootScreen.z > 0)) {
                            
                            // Calculate bounding box from all visible bones
                            float minX = headScreen.x, maxX = headScreen.x;
                            float minY = headScreen.y, maxY = headScreen.y;
                            
                            ExpandBounds(hipsScreen, minX, maxX, minY, maxY);
                            ExpandBounds(leftFootScreen, minX, maxX, minY, maxY);
                            ExpandBounds(rightFootScreen, minX, maxX, minY, maxY);
                            ExpandBounds(leftHandScreen, minX, maxX, minY, maxY);
                            ExpandBounds(rightHandScreen, minX, maxX, minY, maxY);
                            ExpandBounds(leftShoulderScreen, minX, maxX, minY, maxY);
                            ExpandBounds(rightShoulderScreen, minX, maxX, minY, maxY);
                             
                             
                             // Check if Mita is running/moving to make ESP thicker
                             float thickness = 2.0f;
                             int mitaState = sdk::game::GetMitaState();
                             int moveState = sdk::game::GetMitaMovementState();
                             
                             if (mitaState == 1 || moveState >= 2) {
                                 thickness = 4.0f; // Dicker wenn sie läuft
                             }

                             // Add padding above head for hair/accessories
                             float height = maxY - minY;
                             minY -= height * 0.25f; // Increased from 0.1f for better coverage (as requested)
                             
                            // Add horizontal padding for arms/body width
                            float width = maxX - minX;
                            float paddingX = width * 0.15f;
                            minX -= paddingX;
                            maxX += paddingX;
                            
                            // Minimum size check
                            if ((maxY - minY) < 10.0f) {
                                maxY = minY + 10.0f;
                            }
                            if ((maxX - minX) < 10.0f) {
                                float center = (minX + maxX) / 2.0f;
                                minX = center - 5.0f;
                                maxX = center + 5.0f;
                            }
                             
                            // Draw Box
                            ImGui::GetForegroundDrawList()->AddRect(
                                ImVec2(minX, minY), 
                                ImVec2(maxX, maxY), 
                                ImColor(255, 0, 0, 255), 
                                0.0f, 
                                0, 
                                thickness
                            );
                             
                            // Draw Name centered above box
                            float textWidth = ImGui::CalcTextSize("Mita").x;
                            ImGui::GetForegroundDrawList()->AddText(
                                ImVec2(minX + (maxX - minX) / 2.0f - textWidth / 2.0f, minY - 18), 
                                ImColor(255,255,255,255), 
                                "Mita"
                            );
                        }
                    } else {
                        // Fallback: No animator, use simple position-based ESP
                        sdk::Vector3 rootPos = sdk::game::GetPosition(mita);
                        sdk::Vector3 headPos = rootPos;
                        headPos.y += 1.6f; // Estimate height
                        
                        sdk::Vector3 screenRoot = sdk::game::WorldToScreen(rootPos);
                        sdk::Vector3 screenHead = sdk::game::WorldToScreen(headPos);
                        
                        if (screenRoot.z > 0 && screenHead.z > 0) {
                            float height = screenRoot.y - screenHead.y;
                            if (height < 0) height = -height;
                            if (height < 5.0f) height = 5.0f;
                             
                            float width = height * 0.5f;
                            float x = screenHead.x - (width / 2);
                            float y = screenHead.y;
                             
                            ImGui::GetForegroundDrawList()->AddRect(
                                ImVec2(x, y), 
                                ImVec2(x + width, y + height), 
                                ImColor(255, 0, 0, 255), 
                                0.0f, 0, 2.0f
                            );
                             
                            ImGui::GetForegroundDrawList()->AddText(
                                ImVec2(x + (width/2) - 10, y - 15), 
                                ImColor(255,255,255,255), 
                                "Mita"
                            );
                        }
                    }
                }
            }
    }
}
