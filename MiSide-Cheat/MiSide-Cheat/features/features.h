#pragma once
#include <string>
#include <vector>
#include <functional>

// ============================================================
// Feature/Module System for MiSide-Zero
// Clean architecture for adding new features
// ============================================================

namespace features {
    
    // ============================================================
    // Module Base Class
    // Every feature inherits from this
    // ============================================================
    class IModule {
    public:
        virtual ~IModule() = default;
        
        // Module info
        virtual const char* GetName() const = 0;
        virtual const char* GetDescription() const = 0;
        virtual const char* GetCategory() const = 0;
        
        // State
        virtual bool IsEnabled() const = 0;
        virtual void SetEnabled(bool enabled) = 0;
        virtual void Toggle() { SetEnabled(!IsEnabled()); }
        
        // Tick functions
        virtual void OnEnable() {}
        virtual void OnDisable() {}
        virtual void OnTick() {}
        virtual void OnRender() {}
    };
    
    // ============================================================
    // Module Categories
    // ============================================================
    enum class Category {
        Visuals,
        Aimbot,
        Movement,
        Player,
        World,
        Misc
    };
    
    inline const char* CategoryToString(Category cat) {
        switch (cat) {
            case Category::Visuals:  return "Visuals";
            case Category::Aimbot:   return "Aimbot";
            case Category::Movement: return "Movement";
            case Category::Player:   return "Player";
            case Category::World:    return "World";
            case Category::Misc:     return "Misc";
            default: return "Unknown";
        }
    }
    
    // ============================================================
    // Module Info Structure (for UI display)
    // ============================================================
    struct ModuleInfo {
        const char* name;
        const char* category;
        bool* enabled_ptr;
        
        bool IsEnabled() const { return enabled_ptr && *enabled_ptr; }
    };
    
    // ============================================================
    // Get list of all registered modules for display
    // ============================================================
    std::vector<ModuleInfo> GetAllModules();
    std::vector<ModuleInfo> GetEnabledModules();
    
    // ============================================================
    // Initialize all features
    // ============================================================
    void Initialize();
    void Shutdown();
    void OnTick();
    void OnRender();
}
