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
#include <cmath>
#include "../sdk/sdk.h"

namespace features {

    std::vector<ModuleInfo> GetAllModules() {
        std::vector<ModuleInfo> modules;
        
        modules.push_back({ "ESP",           "Visuals",  &config::g_config.visuals.esp.enabled });
        modules.push_back({ "Chams",         "Visuals",  &config::g_config.visuals.chams.enabled });
        modules.push_back({ "Path Predict",  "Visuals",  &config::g_config.visuals.path_prediction });
        modules.push_back({ "Collectibles",  "Visuals",  &config::g_config.visuals.esp_collectibles });
        
        modules.push_back({ "Aimbot",        "Aimbot",   &config::g_config.aimbot.aimbot.enabled });
        
        modules.push_back({ "Speed",         "Movement", &config::g_config.misc.speed_hack.enabled });
        modules.push_back({ "Fly",           "Movement", &config::g_config.misc.fly_hack.enabled });
        modules.push_back({ "NoClip",        "Movement", &config::g_config.misc.no_clip.enabled });
        modules.push_back({ "Freeze",        "Movement", &config::g_config.misc.freeze_cam.enabled });
        
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
        
        std::sort(enabled.begin(), enabled.end(), [](const ModuleInfo& a, const ModuleInfo& b) {
            return strlen(a.name) > strlen(b.name);
        });
        
        return enabled;
    }
    
    void Initialize() {
        static bool init = false;
        if (!init) {
            init = sdk::Initialize();
        }
        
        debug_draw::Initialize();
    }
    
    void Shutdown() {
    }
    
    void DisableUnstableFeatures() {
        config::g_config.visuals.esp.enabled = false;
        config::g_config.visuals.chams.enabled = false;
        config::g_config.aimbot.aimbot.enabled = false;
        config::g_config.misc.speed_hack.enabled = false;
        config::g_config.misc.fly_hack.enabled = false;
        config::g_config.misc.no_clip.enabled = false;
        config::g_config.misc.freeze_cam.enabled = false;
        config::g_config.misc.mita_speed_enabled = false;
        config::g_config.misc.debug_draw_hooks = false;
        config::g_config.misc.debug_draw_render = false;

        debug_draw::DisableHooks();
    }
    
    void OnTick() {
        static bool sdk_ready = false;
        if (!sdk_ready) {
            sdk_ready = sdk::Initialize();
            if (sdk_ready) {
                sdk::AttachCurrentThread();
            }
        }
        if (!sdk_ready) return;
        
        static bool lastNoClip = false;
        static float saved_speed = -1.0f;
        static float saved_mita_speed = -1.0f;
        static bool hooksEnabled = false;

        static bool freecamActive = false;
        static bool freecamWasActive = false;
        static sdk::Vector3 freecamPosition = {0, 0, 0};
        static sdk::Vector3 savedCameraPosition = {0, 0, 0};
        static sdk::Quaternion freecamRotation = {0, 0, 0, 1};
        static sdk::Quaternion savedCameraRotation = {0, 0, 0, 1};
        static float freecamYaw = 0.0f;
        static float freecamPitch = 0.0f;
        
        void* rb = nullptr;
        void* col = nullptr;
        void* move = nullptr;
        void* agent = nullptr;
        void* animator = nullptr;
        
        __try {
            rb = sdk::game::GetPlayerRigidbody();
            col = sdk::game::GetPlayerCollider();
            move = sdk::game::GetPlayerMovement();
            
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

            if (move) {
                if (config::g_config.misc.speed_hack.IsActive()) {
                    if (saved_speed < 0.0f) {
                        float current = sdk::game::GetSpeed(move);
                        if (current > 0.1f && current < 20.0f) {
                             saved_speed = current;
                        } else {
                             saved_speed = 3.0f;
                        }
                    }
                    
                    float target = saved_speed * config::g_config.misc.speed_multiplier;
                    sdk::game::SetSpeed(move, target);
                } else {
                    if (saved_speed > 0.0f) {
                        sdk::game::SetSpeed(move, saved_speed);
                        saved_speed = -1.0f;
                    }
                }
            }

            if (config::g_config.misc.mita_speed_enabled) {
                agent = sdk::game::GetMitaNavMeshAgent();
                animator = sdk::game::GetMitaAnimator();

                if (animator) {
                    sdk::game::SetAnimatorApplyRootMotion(animator, false);
                }

                if (agent) {
                    if (saved_mita_speed < 0.0f) {
                         float currentSpeed = sdk::game::GetAgentSpeed(agent);
                         if (currentSpeed > 0.1f) saved_mita_speed = currentSpeed;
                         else saved_mita_speed = 3.5f;
                    }

                    sdk::game::SetAgentSpeed(agent, config::g_config.misc.mita_speed);
                    sdk::game::SetAgentAcceleration(agent, 99999.0f);
                }
            } else {
                if (saved_mita_speed > 0.0f) {
                     void* anim_restore = sdk::game::GetMitaAnimator();
                     if (anim_restore) sdk::game::SetAnimatorApplyRootMotion(anim_restore, true);

                     void* agent_restore = sdk::game::GetMitaNavMeshAgent();
                     if (agent_restore) {
                         sdk::game::SetAgentSpeed(agent_restore, saved_mita_speed);
                         sdk::game::SetAgentAcceleration(agent_restore, 8.0f);
                     }
                     saved_mita_speed = -1.0f;
                }
            }

            static float saved_fov = -1.0f;
            if (config::g_config.misc.fov_changer.IsActive()) {
                void* cam = sdk::game::GetPlayerCameraObject();
                if (cam) {
                    if (saved_fov < 0.0f) {
                        float current = sdk::game::GetCameraFOV(cam);
                        if (current > 0.1f) saved_fov = current;
                        else saved_fov = 90.0f;
                    }
                    sdk::game::SetCameraFOV(cam, config::g_config.misc.fov_value);
                }
            } else {
                if (saved_fov > 0.0f) {
                    void* cam = sdk::game::GetPlayerCameraObject();
                    if (cam) {
                        sdk::game::SetCameraFOV(cam, saved_fov);
                    }
                    saved_fov = -1.0f;
                }
            }

            bool freezeActive = config::g_config.misc.freeze_cam.IsActive();

            if (freezeActive) {
                if (!freecamWasActive) {
                    if (rb) {
                        sdk::game::SetRigidbodyKinematic(rb, true);
                        if (sdk::IsValidPtr(rb)) {
                            *(sdk::Vector3*)((uintptr_t)rb + 0x1C) = {0, 0, 0};
                        }
                    }
                    if (col) {
                        sdk::game::SetColliderEnabled(col, false);
                    }

                    void* movement = sdk::game::GetPlayerMovement();
                    void* look = sdk::game::GetPlayerLook();
                    if (movement) sdk::game::SetBehaviourEnabled(movement, false);
                    if (look) sdk::game::SetBehaviourEnabled(look, false);

                    freecamWasActive = true;
                }
            } else {
                if (freecamWasActive) {
                    if (rb) sdk::game::SetRigidbodyKinematic(rb, false);
                    if (col) sdk::game::SetColliderEnabled(col, true);

                    void* movement = sdk::game::GetPlayerMovement();
                    void* look = sdk::game::GetPlayerLook();
                    if (movement) sdk::game::SetBehaviourEnabled(movement, true);
                    if (look) sdk::game::SetBehaviourEnabled(look, true);

                    freecamWasActive = false;
                }
            }

            static bool flyWasActive = false;
            static sdk::Vector3 flyPosition = {0, 0, 0};
            static bool flyPositionInitialized = false;
            bool flyActive = config::g_config.misc.fly_hack.IsActive();

            if (flyActive) {
                if (!flyWasActive) {
                    void* feet = sdk::game::GetPlayerFeetTransform();
                    if (feet && sdk::IsValidPtr(feet)) {
                        __try {
                            flyPosition = sdk::game::GetTransformPositionFast(feet);
                            flyPositionInitialized = true;
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER) {
                            void* cam = sdk::game::GetPlayerCamera();
                            if (cam) {
                                flyPosition = sdk::game::GetCameraPosition(cam);
                                flyPositionInitialized = true;
                            }
                        }
                    }

                    void* move = sdk::game::GetPlayerMovement();
                    if (move && sdk::IsValidPtr(move)) {
                        __try {
                            *(bool*)((uintptr_t)move + 0x18) = false;
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER) {}
                    }

                    if (rb && sdk::IsValidPtr(rb)) {
                        sdk::game::SetRigidbodyKinematic(rb, true);
                    }
                    if (col && sdk::IsValidPtr(col)) {
                        sdk::game::SetColliderEnabled(col, false);
                    }

                    flyWasActive = true;
                }

                if (flyPositionInitialized) {
                    void* cam = sdk::game::GetPlayerCamera();
                    if (cam && sdk::IsValidPtr(cam)) {
                        __try {
                            sdk::Vector3 forward = sdk::game::GetCameraForward(cam);

                            float forwardLen = sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
                            if (forwardLen > 0.001f) {
                                forward.x /= forwardLen;
                                forward.y /= forwardLen;
                                forward.z /= forwardLen;
                            }

                            sdk::Vector3 right = {-forward.z, 0.0f, forward.x};
                            float rightLen = sqrtf(right.x * right.x + right.z * right.z);
                            if (rightLen > 0.001f) {
                                right.x /= rightLen;
                                right.z /= rightLen;
                            }

                            float speed = config::g_config.misc.fly_speed * ImGui::GetIO().DeltaTime;

                            sdk::Vector3 moveDelta = {0, 0, 0};

                            if (GetAsyncKeyState(0x57) & 0x8000) {
                                moveDelta.x += forward.x * speed;
                                moveDelta.y += forward.y * speed;
                                moveDelta.z += forward.z * speed;
                            }
                            if (GetAsyncKeyState(0x53) & 0x8000) {
                                moveDelta.x -= forward.x * speed;
                                moveDelta.y -= forward.y * speed;
                                moveDelta.z -= forward.z * speed;
                            }
                            if (GetAsyncKeyState(0x41) & 0x8000) {
                                moveDelta.x -= right.x * speed;
                                moveDelta.z -= right.z * speed;
                            }
                            if (GetAsyncKeyState(0x44) & 0x8000) {
                                moveDelta.x += right.x * speed;
                                moveDelta.z += right.z * speed;
                            }
                            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                                moveDelta.y += speed;
                            }
                            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                                moveDelta.y -= speed;
                            }

                            flyPosition.x += moveDelta.x;
                            flyPosition.y += moveDelta.y;
                            flyPosition.z += moveDelta.z;

                            void* feet = sdk::game::GetPlayerFeetTransform();
                            if (feet && sdk::IsValidPtr(feet)) {
                                sdk::game::SetPlayerPositionDirect(flyPosition);
                            }
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER) {
                        }
                    }
                }
            } else {
                if (flyWasActive) {
                    void* move = sdk::game::GetPlayerMovement();
                    if (move && sdk::IsValidPtr(move)) {
                        __try {
                            *(bool*)((uintptr_t)move + 0x18) = true;
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER) {}
                    }

                    if (rb && sdk::IsValidPtr(rb)) {
                        sdk::game::SetRigidbodyKinematic(rb, false);
                    }
                    if (col && sdk::IsValidPtr(col)) {
                        sdk::game::SetColliderEnabled(col, true);
                    }

                    flyWasActive = false;
                    flyPositionInitialized = false;
                }
            }

            chams::OnTick();
            
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
        }
        
        __try {
            game_cache::UpdateCache();
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    void OnRender() {
        RenderPathPrediction();
        
        esp::RenderAll();
        
        debug_view::Render();
        
        if (config::g_config.misc.debug_draw_render && debug_draw::IsEnabled()) {
            std::vector<features::debug_draw::DebugLine> lines;
            std::vector<features::debug_draw::DebugRay> rays;
            
            __try {
                lines = debug_draw::GetLines();
                rays = debug_draw::GetRays();
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
            }
            
            if (!lines.empty() || !rays.empty()) {
                int lineCount = 0;
                int maxLines = config::g_config.misc.debug_draw_max_lines;
                
                for (const auto& line : lines) {
                    if (lineCount >= maxLines) break;
                    
                    sdk::Vector3 screenStart = sdk::game::WorldToScreen(line.start);
                    sdk::Vector3 screenEnd = sdk::game::WorldToScreen(line.end);
                    
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
                
                int rayCount = 0;
                int maxRays = config::g_config.misc.debug_draw_max_rays;
                
                for (const auto& ray : rays) {
                    if (rayCount >= maxRays) break;
                    
                    sdk::Vector3 rayEnd = ray.start + (ray.direction * 10.0f);
                    
                    sdk::Vector3 screenStart = sdk::game::WorldToScreen(ray.start);
                    sdk::Vector3 screenEnd = sdk::game::WorldToScreen(rayEnd);
                    
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
