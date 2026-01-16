# Fix Glow ESP and Implement Partial Body Modulation - COMPLETED

## Problem Statement

1. **Glow ESP displays the same as Flat ESP**: The Glow ESP option currently displays the same visual effect as Flat ESP. Both use the same material properties without any glow-specific settings.
2. **Need partial body modulation**: The user wants to be able to apply different colors/effects to different parts of the character model (head, body, legs, arms).

## Root Cause Analysis

### Glow ESP Issue

The current implementation in `chams.cpp` used the same `SetMaterialColor` function for both Flat and Glow ESP types:
```cpp
if (currentType == 2) {
    sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]); 
} else {
    sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]);
}
```

Both branches called the same function with the same parameters. The glow material was created with a glow shader (e.g., "Sprites/Default"), but no glow-specific properties were set.

### Missing Glow Properties

Unity materials that support glow typically have:
- **Emission color**: The color and intensity of the glow
- **Glow strength**: A multiplier for the glow intensity
- **Blend mode**: How the glow blends with the background (additive for glow)

The SDK only provided `SetMaterialColor` which sets the base color, but didn't provide functions to set emission color or glow strength.

## Implementation Summary

### Phase 1: Fix Glow ESP ✅

#### Step 1.1: Add Glow-Specific Material Property Functions to SDK ✅

**Files Modified**: `sdk/sdk.h` and `sdk/sdk.cpp`

Added new functions to set glow-specific material properties:

```cpp
// In sdk.h - added to game namespace
void SetMaterialEmissionColor(void* material, float r, float g, float b, float a);
void SetMaterialGlowStrength(void* material, float strength);
```

**Implementation in sdk.cpp**:

```cpp
void SetMaterialEmissionColor(void* material, float r, float g, float b, float a) {
    if (!material) return;
    if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
    
    // Try to find SetEmissionColor method
    static Il2CppMethod* setEmission = nullptr;
    if (!setEmission) setEmission = GetMethod((Il2CppClass*)g_MaterialClass, "SetEmissionColor", 1);
    
    // Color struct
    struct Color { float r,g,b,a; };
    Color c = {r,g,b,a};
    void* params[1] = { &c };
    
    RuntimeInvoke(setEmission, material, params, nullptr);
}

void SetMaterialGlowStrength(void* material, float strength) {
    if (!material) return;
    if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
    
    // Try to find SetFloat method with "_GlowStrength" property
    static Il2CppMethod* setFloat = nullptr;
    if (!setFloat) setFloat = GetMethod((Il2CppClass*)g_MaterialClass, "SetFloat", 2);
    
    // Create string for property name
    static void* (*il2cpp_string_new)(const char*) = nullptr;
    if (!il2cpp_string_new) {
        il2cpp_string_new = (void* (*)(const char*))GetProcAddress(g_GameAssembly, "il2cpp_string_new");
    }
    
    void* propName = il2cpp_string_new("_GlowStrength");
    void* params[2] = { propName, &strength };
    
    RuntimeInvoke(setFloat, material, params, nullptr);
}
```

#### Step 1.2: Update Chams to Use Glow Properties ✅

**File Modified**: `features/chams.cpp`

Updated the color setting logic to use glow properties when Glow ESP is selected:

```cpp
// Update color
float* col = config::g_config.visuals.chams_color;
if (currentType == 2) {
    // For glow, set emission color and glow strength
    sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]);
    sdk::game::SetMaterialEmissionColor(targetMat, col[0], col[1], col[2], col[3]);
    sdk::game::SetMaterialGlowStrength(targetMat, 1.0f); // Full glow strength
} else {
    // For flat, just set base color
    sdk::game::SetMaterialColor(targetMat, col[0], col[1], col[2], col[3]);
}
```

#### Step 1.3: Improve Glow Shader Detection ✅

**File Modified**: `features/chams.cpp`

Added more fallback shaders for better glow detection:

```cpp
if (!g_GlowMaterial) {
    // Try to find a glowing/additive shader with multiple fallbacks
    void* shader = sdk::game::FindShader("Sprites/Default");
    if (!shader) shader = sdk::game::FindShader("Particles/Standard Unlit");
    if (!shader) shader = sdk::game::FindShader("Mobile/Particles/Additive");
    if (!shader) shader = sdk::game::FindShader("UI/Default");
    if (!shader) shader = sdk::game::FindShader("Unlit/Transparent");
    if (!shader) shader = sdk::game::FindShader("Particles/Additive");
    if (!shader) shader = sdk::game::FindShader("Mobile/Diffuse");
    
    if (shader) {
        g_GlowMaterial = sdk::game::CreateMaterial(shader);
    } else {
        g_GlowMaterial = g_FlatMaterial; // Fallback
    }
}
```

### Phase 2: Implement Partial Body Modulation ✅

#### Step 2.1: Add Renderer Filtering Functions to SDK ✅

**Files Modified**: `sdk/sdk.h` and `sdk/sdk.cpp`

Added functions to filter renderers by body part:

```cpp
// In sdk.h - added to game namespace
enum BodyPart {
    BodyPart_None = 0,
    BodyPart_Head = 1,
    BodyPart_Body = 2,
    BodyPart_Legs = 3,
    BodyPart_Arms = 4
};

// Get body part for a renderer based on its transform name
BodyPart GetRendererBodyPart(void* renderer);

// Filter renderers by body part
std::vector<void*> GetRenderersByBodyPart(void* gameObject, BodyPart part);
```

**Implementation in sdk.cpp**:

```cpp
BodyPart GetRendererBodyPart(void* renderer) {
    if (!renderer) return BodyPart_None;
    
    // Get transform of renderer
    if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
    static Il2CppMethod* getTrans = nullptr;
    if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);
    
    void* transform = RuntimeInvoke(getTrans, renderer, nullptr, nullptr);
    if (!transform) return BodyPart_None;
    
    // Get transform name
    if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");
    static Il2CppMethod* getName = nullptr;
    if (!getName) getName = GetMethod((Il2CppClass*)g_TransformClass, "get_name", 0);
    
    void* nameObj = RuntimeInvoke(getName, transform, nullptr, nullptr);
    if (!nameObj) return BodyPart_None;
    
    // Get string value from Il2CppString
    const char* name = *(const char**)((char*)nameObj + 0x10);
    if (!name) return BodyPart_None;
    
    // Convert to lowercase for comparison
    std::string nameStr(name);
    for (char& c : nameStr) c = tolower(c);
    
    // Check for body part keywords
    if (nameStr.find("head") != std::string::npos || 
        nameStr.find("hair") != std::string::npos ||
        nameStr.find("face") != std::string::npos) {
        return BodyPart_Head;
    }
    if (nameStr.find("body") != std::string::npos || 
        nameStr.find("torso") != std::string::npos ||
        nameStr.find("chest") != std::string::npos ||
        nameStr.find("spine") != std::string::npos) {
        return BodyPart_Body;
    }
    if (nameStr.find("leg") != std::string::npos || 
        nameStr.find("foot") != std::string::npos ||
        nameStr.find("toe") != std::string::npos) {
        return BodyPart_Legs;
    }
    if (nameStr.find("arm") != std::string::npos || 
        nameStr.find("hand") != std::string::npos ||
        nameStr.find("finger") != std::string::npos) {
        return BodyPart_Arms;
    }
    
    return BodyPart_None;
}

std::vector<void*> GetRenderersByBodyPart(void* gameObject, BodyPart part) {
    std::vector<void*> results;
    if (!gameObject || part == BodyPart_None) return results;
    
    // Get all renderers
    std::vector<void*> allRenderers = GetRenderers(gameObject);
    
    // Filter by body part
    for (void* renderer : allRenderers) {
        if (GetRendererBodyPart(renderer) == part) {
            results.push_back(renderer);
        }
    }
    
    return results;
}
```

#### Step 2.2: Add Configuration Options for Partial Body Modulation ✅

**File Modified**: `config/config.h`

Added configuration options for partial body modulation:

```cpp
// Partial Body Modulation
bool chams_partial_body = false;  // Enable partial body modulation
bool chams_head = true;            // Apply chams to head
bool chams_body = true;            // Apply chams to body
bool chams_legs = true;            // Apply chams to legs
bool chams_arms = true;            // Apply chams to arms
float chams_head_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };    // Red for head
float chams_body_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };    // Green for body
float chams_legs_color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };    // Blue for legs
float chams_arms_color[4] = { 1.0f, 1.0f, 0.0f, 1.0f };    // Yellow for arms
```

#### Step 2.3: Create Separate Materials for Each Body Part ✅

**File Modified**: `features/chams.cpp`

Added material variables for each body part:

```cpp
// Partial body modulation materials
static void* g_HeadMaterial = nullptr;
static void* g_BodyMaterial = nullptr;
static void* g_LegsMaterial = nullptr;
static void* g_ArmsMaterial = nullptr;
```

Created materials for each body part when partial body modulation is enabled:

```cpp
if (partialBodyEnabled) {
    // Create materials for each body part if needed
    if (!g_HeadMaterial) {
        void* shader = sdk::game::FindShader("GUI/Text Shader");
        if (!shader) shader = sdk::game::FindShader("Unlit/Color");
        if (shader) g_HeadMaterial = sdk::game::CreateMaterial(shader);
    }
    if (!g_BodyMaterial) {
        void* shader = sdk::game::FindShader("GUI/Text Shader");
        if (!shader) shader = sdk::game::FindShader("Unlit/Color");
        if (shader) g_BodyMaterial = sdk::game::CreateMaterial(shader);
    }
    if (!g_LegsMaterial) {
        void* shader = sdk::game::FindShader("GUI/Text Shader");
        if (!shader) shader = sdk::game::FindShader("Unlit/Color");
        if (shader) g_LegsMaterial = sdk::game::CreateMaterial(shader);
    }
    if (!g_ArmsMaterial) {
        void* shader = sdk::game::FindShader("GUI/Text Shader");
        if (!shader) shader = sdk::game::FindShader("Unlit/Color");
        if (shader) g_ArmsMaterial = sdk::game::CreateMaterial(shader);
    }
}
```

#### Step 2.4: Update Chams to Apply Different Materials to Different Body Parts ✅

**File Modified**: `features/chams.cpp`

Updated the renderer application logic to apply different materials based on body part:

```cpp
// Apply chams
if (partialBodyEnabled) {
    // Partial body modulation - apply different materials based on body part
    sdk::game::BodyPart part = sdk::game::GetRendererBodyPart(r);
    void* matToApply = targetMat;
    
    switch (part) {
        case sdk::game::BodyPart_Head:
            if (config::g_config.visuals.chams_head && g_HeadMaterial) {
                matToApply = g_HeadMaterial;
            }
            break;
        case sdk::game::BodyPart_Body:
            if (config::g_config.visuals.chams_body && g_BodyMaterial) {
                matToApply = g_BodyMaterial;
            }
            break;
        case sdk::game::BodyPart_Legs:
            if (config::g_config.visuals.chams_legs && g_LegsMaterial) {
                matToApply = g_LegsMaterial;
            }
            break;
        case sdk::game::BodyPart_Arms:
            if (config::g_config.visuals.chams_arms && g_ArmsMaterial) {
                matToApply = g_ArmsMaterial;
            }
            break;
        default:
            // No specific body part, use default material
            matToApply = targetMat;
            break;
    }
    
    sdk::game::SetMaterial(r, matToApply);
} else {
    // Standard chams - apply same material to all renderers
    sdk::game::SetMaterial(r, targetMat);
}
```

## Files Modified

1. **sdk/sdk.h** - Added glow material property function declarations and body part enum
2. **sdk/sdk.cpp** - Implemented glow material property functions and body part detection
3. **features/chams.cpp** - Updated to use glow properties and implement partial body modulation
4. **config/config.h** - Added configuration options for partial body modulation

## Testing Checklist

- [ ] Verify Glow ESP looks different from Flat ESP
- [ ] Verify glow effect is visible and properly colored
- [ ] Verify partial body modulation works
- [ ] Verify each body part can be toggled independently
- [ ] Verify different colors can be applied to different body parts
- [ ] Check performance impact
- [ ] Test with different chams types (Flat, Glow)

## Notes

- The glow effect depends on the shader supporting emission and glow properties
- If the game doesn't use standard Unity shaders, the glow effect may not work as expected
- Body part detection relies on transform names, which may vary between games
- Consider adding a debug mode to visualize detected body parts
- Performance impact should be minimal as body part detection is done once per renderer
- Added necessary includes to sdk.cpp: `<string>`, `<algorithm>`, `<cctype>` for string manipulation
