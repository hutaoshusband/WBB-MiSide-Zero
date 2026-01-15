#pragma once
#include <Windows.h>

namespace config {
    struct Bind {
        bool enabled = false;
        int key = 0;
        int mode = 2; // 0=Hold, 1=Toggle, 2=Always On (Default to Always On)
        bool active = false;
        bool key_was_down = false;

        bool IsActive() {
            if (!enabled) return false;
            if (mode == 2) return true; // Always On
            
            bool key_down = (key != 0 && (GetAsyncKeyState(key) & 0x8000));
            
            if (mode == 0) { // Hold
                active = key_down;
            } else if (mode == 1) { // Toggle
                if (key_down && !key_was_down) {
                    active = !active;
                }
                key_was_down = key_down;
            }
            
            return active;
        }

        // Helper for legacy code that just wants a pointer to bool
        bool* GetEnabledPtr() { return &enabled; }
    };
}
