# Debug Draw Hooks Implementation Summary

## Overview
Successfully implemented hooks for Unity's `Debug.DrawLine` and `Debug.DrawRay` functions to capture and visualize debug information from the MiSide game. This allows you to see AI paths, debug lines, and other developer visualizations that may have been left in the game.

## What Was Implemented

### 1. Debug Draw System (`features/debug_draw.h` & `.cpp`)
**Purpose:** Hook Unity debug drawing functions and capture their data

**Features:**
- **Hook Management:** Hooks `UnityEngine.Debug.DrawLine` and `UnityEngine.Debug.DrawRay` using MinHook
- **Data Capture:** Stores captured debug lines and rays with their properties (start, end, direction, color, duration, depthTest)
- **Thread-Safe Storage:** Uses mutex for safe multi-threaded access to captured data
- **Statistics Tracking:** Counts captured lines, rays, and Gizmos calls
- **Buffer Management:** Limits storage to last 1000 lines/rays to prevent memory issues
- **Original Function Preservation:** Calls original functions to maintain game behavior

**Key Functions:**
- `Initialize()` - Initialize MinHook and the debug draw system
- `EnableHooks()` / `DisableHooks()` - Manage hook state
- `GetLines()` / `GetRays()` - Retrieve captured data (thread-safe)
- `GetStats()` - Get capture statistics
- `ClearStats()` - Reset statistics counter

### 2. Configuration (`config/config.h`)
Added new settings to `MiscSettings`:
- `debug_draw_hooks` (bool) - Enable/disable the debug hooks
- `debug_draw_render` (bool) - Render captured debug lines on screen
- `debug_draw_max_lines` (int) - Maximum number of lines to render (100-2000)
- `debug_draw_max_rays` (int) - Maximum number of rays to render (100-2000)

### 3. Menu Integration (`render/ui/menu.h`)
Added new "Debug" sub-tab in the Misc section:

**Features:**
- Enable/Disable debug hooks toggle
- Render debug lines toggle
- Max lines slider (100-2000)
- Max rays slider (100-2000)
- Real-time statistics display:
  - Hooks Active status
  - Lines Captured count
  - Rays Captured count
  - Gizmos Called count
- Clear Statistics button

### 4. Features Integration (`features/features.cpp`)
**Initialization:**
- Automatically initializes debug draw system on cheat startup
- Manages hook state based on config settings

**Rendering:**
- Renders captured debug lines and rays as overlays
- Converts world coordinates to screen space
- Uses original colors from debug calls
- Limits rendering to configured maximums
- SEH exception handling for safety

**Debug View (F7):**
- Shows real-time statistics
- Displays current settings
- Helps verify that hooks are working

## How It Works

### Hook Process
1. When enabled, MinHook intercepts calls to `UnityEngine.Debug.DrawLine` and `UnityEngine.Debug.DrawRay`
2. The hooked function captures all parameters:
   - Start position (Vector3)
   - End position or direction (Vector3)
   - Color (Color)
   - Duration (float)
   - Depth test (bool)
3. Stores the data in thread-safe buffers (max 1000 each)
4. Calls the original function to maintain game behavior
5. Increments statistics counters

### Rendering Process
1. On each frame, if rendering is enabled:
   - Retrieves captured lines and rays from buffers
   - Converts 3D world coordinates to 2D screen coordinates
   - Draws lines using ImGui's foreground draw list
   - Respects configured maximums (max lines/rays)
   - Uses original colors from debug calls

### Use Case: ESP for Debug Visualization
This is essentially an "ESP" for developer debug visualizations:
- **AI Paths:** See where AI entities (like Mita) are walking/running
- **Sight Lines:** See enemy line-of-sight checks
- **Projectile Paths:** See where bullets would travel
- **Hitboxes:** See debug collision boxes (if developers use debug drawing for them)
- **Pathfinding:** See navigation mesh paths

## Usage

### In-Game
1. Press INSERT to open the cheat menu
2. Go to "Misc" tab → "Debug" sub-tab
3. Toggle "Enable Debug Hooks"
4. Toggle "Render Debug Lines" (optional, to see visualization)
5. Adjust "Max Lines" and "Max Rays" sliders as needed
6. Press F7 to open Debug View and see statistics

### Understanding Results
- **Lines Captured:** Number of Debug.DrawLine calls
- **Rays Captured:** Number of Debug.DrawRay calls
- **Gizmos Called:** Number of OnDrawGizmos calls (if implemented)
- **Hooks Active:** YES if hooks are currently enabled

### What You Might See
If the game uses debug drawing:
- **Colored lines** representing AI movement paths
- **Direction rays** showing where enemies are looking
- **Box/circle visualizations** for collision hitboxes
- **Navigation lines** for pathfinding

### If Nothing Appears
This is normal if:
- The game doesn't call these functions in release builds
- Developers removed debug drawing from release builds
- Debug drawing is only enabled in debug/editor builds

## Technical Details

### Hook Targets
Based on dump.cs analysis:
- `UnityEngine.Debug.DrawLine(Vector3 start, Vector3 end, Color color, float duration, bool depthTest)`
- `UnityEngine.Debug.DrawRay(Vector3 start, Vector3 dir, Color color, float duration, bool depthTest)`

### MinHook Integration
- Uses MinHook library for function hooking
- Thread-safe hooking with proper cleanup
- Original function pointers stored and called
- Hooks can be enabled/disabled at runtime

### Thread Safety
- Mutex protects all data access
- GetLines() and GetRays() return copies of data
- Statistics counters use atomic operations

### Performance Considerations
- Limits buffer size to 1000 items each
- Configurable rendering limits
- Efficient data structures (vectors)
- Minimal overhead when hooks are disabled

## Files Modified/Created

### Created:
- `MiSide-Cheat/MiSide-Cheat/features/debug_draw.h` - Header file
- `MiSide-Cheat/MiSide-Cheat/features/debug_draw.cpp` - Implementation

### Modified:
- `MiSide-Cheat/MiSide-Cheat/config/config.h` - Added debug draw settings
- `MiSide-Cheat/MiSide-Cheat/features/features.cpp` - Integration and rendering
- `MiSide-Cheat/MiSide-Cheat/features/features.h` - Forward declarations
- `MiSide-Cheat/MiSide-Cheat/render/ui/menu.h` - UI integration
- `MiSide-Cheat/MiSide-Cheat/MiSide-Cheat.vcxproj` - Project file

## Future Enhancements

Potential improvements:
1. **OnDrawGizmos Hooking** - Hook MonoBehaviour.OnDrawGizmos for more debug info
2. **Filter Options** - Filter by color, duration, or object
3. **Data Export** - Export captured debug data to file
4. **Visualization Options** - Toggle specific types of debug visualization
5. **Ray Length Configuration** - Adjust ray rendering length
6. **Color Override** - Force specific colors for captured lines

## Troubleshooting

**Q: No debug lines appear even though hooks are enabled**
A: The game may not call Debug.DrawLine/DrawRay in release builds. This is common in Unity games.

**Q: Statistics show 0 captured**
A: Same as above - the game doesn't use these debug functions, or they're compiled out.

**Q: Game crashes when enabling hooks**
A: The function signatures might be incorrect. Check dump.cs for the exact function signatures and offsets.

**Q: Lines appear but in wrong positions**
A: World-to-screen conversion might need adjustment. Check the SDK's WorldToScreen implementation.

## Conclusion

This implementation provides a powerful debugging tool that can reveal hidden developer visualizations in the game. While it may not work in all games (especially those with well-optimized release builds), when it does work, it provides invaluable insight into game mechanics, AI behavior, and collision systems.

The system is designed to be:
- **Non-invasive:** Calls original functions to maintain game behavior
- **Performant:** Minimal overhead when disabled
- **Thread-safe:** Safe for multi-threaded environments
- **Configurable:** User can adjust behavior and limits
- **Informative:** Statistics help verify functionality
