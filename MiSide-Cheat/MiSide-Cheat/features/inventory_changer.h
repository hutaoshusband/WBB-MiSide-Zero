#pragma once
#include <vector>
#include <string>

namespace features {
    namespace inventory {
        void RenderWindow();
        void Update();
        
        // List of known items to populate the cheat menu
        extern std::vector<std::string> known_items;
        
        // Window state
        extern bool show_window;
    }
}
