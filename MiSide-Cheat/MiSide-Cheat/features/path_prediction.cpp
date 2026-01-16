#include "path_prediction.h"
#include "../sdk/sdk.h"
#include "../config/config.h"
#include <imgui.h>
#include <vector>

namespace features {
    namespace path_prediction {
        
        void OnRender() {
            // Check if feature is enabled
            if (!config::g_config.visuals.path_prediction) return;
            
            // Check if SDK is ready
            if (!sdk::IsReady()) return;

            // Get active tweens
            // Note: This iterates the TweenManager logic we implemented in SDK
            std::vector<void*> tweens = sdk::game::GetActiveTweens();
            if (tweens.empty()) return;

            ImDrawList* draw = ImGui::GetForegroundDrawList();

            for (void* tween : tweens) {
                if (!tween) continue;
                
                // Try to extract path points from this tween
                // This will fail (return empty) for non-path tweens, effectively filtering them
                std::vector<sdk::Vector3> points = sdk::game::GetTweenPathPoints(tween);
                if (points.size() < 2) continue;

                // Draw the path
                // We draw lines connecting the points
                for (size_t i = 0; i < points.size() - 1; i++) {
                     sdk::Vector3 p1 = points[i];
                     sdk::Vector3 p2 = points[i+1];
                     
                     sdk::Vector3 s1 = sdk::game::WorldToScreen(p1);
                     sdk::Vector3 s2 = sdk::game::WorldToScreen(p2);
                     
                     // Check visibility (simple Z check)
                     if (s1.z > 0 && s2.z > 0) {
                         // Draw line segment
                         draw->AddLine(ImVec2(s1.x, s1.y), ImVec2(s2.x, s2.y), ImColor(0, 255, 255, 200), 2.0f);
                     }
                     
                     // Draw point marker
                     if (s1.z > 0) {
                         draw->AddCircleFilled(ImVec2(s1.x, s1.y), 3.0f, ImColor(255, 255, 0, 200));
                         
                         // Optional: Numbering
                         // char buf[16]; sprintf(buf, "%d", (int)i);
                         // draw->AddText(ImVec2(s1.x, s1.y), ImColor(255,255,255), buf);
                     }
                }
                
                // Draw last point distinctively
                if (!points.empty()) {
                    sdk::Vector3 end = points.back();
                    sdk::Vector3 sEnd = sdk::game::WorldToScreen(end);
                    if (sEnd.z > 0) {
                        draw->AddCircleFilled(ImVec2(sEnd.x, sEnd.y), 5.0f, ImColor(255, 0, 0, 255));
                        draw->AddText(ImVec2(sEnd.x + 8, sEnd.y - 4), ImColor(255, 255, 255, 255), "End");
                    }
                }
                
                // Only draw first few paths to avoid clutter? 
                // No, draw all found.
            }
        }
    }
}
