#include "path_prediction.h"
#include "../sdk/sdk.h"
#include "../config/config.h"
#include <imgui.h>

namespace features {

    void RenderPathPrediction() {
        if (!config::g_config.visuals.path_prediction) return;

        // Path prediction for Rigidbody objects (Projectiles)
        // We scan for Rigidbodies that are moving
        
        // Since iterating all rigidbodies is expensive, we might want to cache or optimize.
        // For now, let's try finding objects of type "Rigidbody"
        // But "Rigidbody" is a Component. FindObjectsOfTypeAll handles Components too usually if we ask for the Type.
        
        std::vector<void*> rigidbodies = sdk::game::FindObjectsOfTypeAll("UnityEngine.Rigidbody");
        
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        for (void* rb : rigidbodies) {
            if (!rb) continue;
            
            // Filter out static or sleeping bodies
            // We need a way to check if it's sleeping or kinematic? 
            // For now just check velocity.
            
            sdk::Vector3 velocity = sdk::game::GetRigidbodyVelocity(rb);
            if (velocity.x == 0 && velocity.y == 0 && velocity.z == 0) continue;
            
            // Check magnitude to filter noise
            float speed = sqrt(velocity.x*velocity.x + velocity.y*velocity.y + velocity.z*velocity.z);
            if (speed < 0.1f) continue;

            // Get Position
            // Rigidbody is a Component, so we can use GetPosition(rb) which calls GetComponent<Transform>().position
            sdk::Vector3 startPos = sdk::game::GetPosition(rb);
            
            // Simulate path
            // P_t = P_0 + V*t + 0.5*G*t^2
            // Let's draw for 2 seconds
            
            sdk::Vector3 currentPos = startPos;
            sdk::Vector3 g = { 0, -9.81f, 0 }; // Gravity
            float timeStep = 0.1f;
            
            for (float t = 0; t < 2.0f; t += timeStep) {
                // Future position at t + timeStep
                float futureT = t + timeStep;
                
                // Simple integration: P = P0 + V*t + 0.5*G*t^2
                // We restart from P0 for better accuracy (analytical solution) rather than iterative Euler
                
                sdk::Vector3 futurePos;
                futurePos.x = startPos.x + velocity.x * futureT + 0.5f * g.x * futureT * futureT;
                futurePos.y = startPos.y + velocity.y * futureT + 0.5f * g.y * futureT * futureT;
                futurePos.z = startPos.z + velocity.z * futureT + 0.5f * g.z * futureT * futureT;
                
                // Draw Line
                sdk::Vector3 s1 = sdk::game::WorldToScreen(currentPos);
                sdk::Vector3 s2 = sdk::game::WorldToScreen(futurePos);
                
                if (s1.z > 0 && s2.z > 0) {
                    drawList->AddLine({s1.x, s1.y}, {s2.x, s2.y}, IM_COL32(0, 255, 255, 255), 2.0f);
                }
                
                currentPos = futurePos;
            }
        }
    }
}
