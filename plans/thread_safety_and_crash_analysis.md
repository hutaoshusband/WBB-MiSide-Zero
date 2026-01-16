# Thread Safety and Crash Analysis - WBB MiSide-Zero Cheat

## Executive Summary
The cheat has several critical thread safety issues and memory access problems that can cause random crashes. Below is a detailed analysis of all issues found.

---

## 🔴 CRITICAL ISSUES

### 1. **Multiple Thread Attachment to IL2CPP Runtime**

**Location:** `core/core.cpp`, `features/features.cpp`, `sdk/sdk.cpp`

**Problem:**
- Main thread (`core::MainThread`) attaches to IL2CPP runtime
- Render thread (called from `hkPresent`) also tries to attach in `features::OnRender()`
- IL2CPP runtime has strict thread requirements - unattached threads can cause GC crashes

**Impact:**
- "Fatal error in GC: Collecting from unknown thread"
- Random crashes during garbage collection
- Memory corruption

**Evidence:**
```cpp
// core/core.cpp - Main thread
sdk::AttachCurrentThread();

// features/features.cpp - Render thread tries to attach again
if (!renderThreadAttached && sdk::IsReady()) {
    sdk::AttachCurrentThread();
    renderThreadAttached = true;
}
```

---

### 2. **Race Conditions in Static Pointer Caching**

**Location:** `sdk/sdk.cpp` - Multiple functions

**Problem:**
- SDK functions use static variables to cache pointers/classes
- Multiple threads can read/write these simultaneously without synchronization
- Examples:
  ```cpp
  static Il2CppClass* g_PlayerManagerClass = nullptr;
  static Il2CppField* camField = nullptr;
  static void* g_CachedMitaCycle = nullptr;
  ```

**Impact:**
- One thread writes while another reads corrupted data
- Can lead to crashes when accessing invalid pointers

---

### 3. **Unsafe Raw Memory Access Without Validation**

**Location:** `sdk/sdk.cpp` - Multiple locations

**Problem:**
- Direct memory offset reads without validation
- Even if `IsValidPtr()` checks pass, subsequent reads aren't protected
- Examples:
  ```cpp
  // PlayerManager.move at 0x30
  void* move = *(void**)((uintptr_t)pm + 0x30);
  
  // MitaManager.state at 0x80
  int state = *(int*)((uintptr_t)mita + 0x80);
  
  // List<T> structure reads
  int count = *(int*)((uintptr_t)list + 0x18);
  void** items = *(void***)((uintptr_t)list + 0x10);
  ```

**Impact:**
- Crash if object layout changes
- Crash if pointer becomes invalid between check and access

---

### 4. **Hardcoded Hook Addresses Without Base Address Resolution**

**Location:** `features/debug_draw.cpp`

**Problem:**
```cpp
void* debugLineAddr = (void*)0x1801911220; // Hardcoded!
void* debugRayAddr = (void*)0x1801911480;
```
- These are likely offsets, not absolute addresses
- If ASLR is enabled (which it is by default), these addresses are wrong
- Hooking wrong addresses can cause immediate crash

**Impact:**
- Immediate crash when enabling debug draw hooks
- Hooking random memory locations

---

### 5. **Render Target Recreation Without Validation**

**Location:** `hooks/hooks.cpp` - `hkResizeBuffers`

**Problem:**
```cpp
HRESULT __stdcall hkResizeBuffers(...) {
    render::InvalidateDeviceObjects();
    
    HRESULT hr = oResizeBuffers(...);
    
    render::CreateDeviceObjects();
    
    return hr;
}
```
- No validation that device context is still valid
- No check if resize failed
- If resize fails, we try to recreate on invalid device

**Impact:**
- Crash if resize fails
- Invalid device state

---

### 6. **Missing ImGui IO Validation**

**Location:** `sdk/sdk.cpp` - `WorldToScreen`

**Problem:**
```cpp
ImGuiIO& io = ImGui::GetIO();
Vector3 screen = {
    (ndcX + 1.0f) * 0.5f * io.DisplaySize.x,
    (1.0f - ndcY) * 0.5f * io.DisplaySize.y,
    ndcZ
};
```
- Assumes ImGui is initialized
- No null check on io.DisplaySize

**Impact:**
- Crash if called before ImGui init
- Access violation

---

## ⚠️ HIGH PRIORITY ISSUES

### 7. **Excessive Exception Suppression**

**Location:** `core/core.cpp`, `features/features.cpp`

**Problem:**
```cpp
__except(EXCEPTION_EXECUTE_HANDLER) {
    // Silently ignores ALL exceptions
}

static int crashCount = 0;
crashCount++;
if (crashCount > 10) {
    WriteCrashLog("Too many OnTick crashes");
    // But continues to run!
}
```

**Impact:**
- Hides real bugs
- Continues running in broken state
- Cascading failures

---

### 8. **No Thread Detachment on Shutdown**

**Location:** `sdk/sdk.cpp`

**Problem:**
- Threads are attached but never detached
- IL2CPP requires proper thread detachment on exit

**Impact:**
- Resource leaks
- Potential crash during shutdown
- Memory corruption

---

### 9. **Mutex Held During Vector Copy**

**Location:** `features/debug_draw.cpp`

**Problem:**
```cpp
std::vector<DebugLine> GetLines() {
    std::lock_guard<std::mutex> lock(data_mutex);
    return captured_lines; // Makes copy while locked!
}
```

**Impact:**
- Blocks render thread while copying
- Frame drops
- If vector is large, significant performance impact

---

### 10. **SEH Not Supported on All Compilers**

**Location:** Multiple files

**Problem:**
```cpp
__try {
    // code
}
__except(EXCEPTION_EXECUTE_HANDLER) {
    // handler
}
```
- Structured Exception Handling (SEH) is Microsoft-specific
- May not work with all compiler configurations
- Can cause undefined behavior

**Impact:**
- May not catch exceptions as expected
- Compiler warnings/errors

---

## 🔵 MEDIUM PRIORITY ISSUES

### 11. **Inconsistent Use of IsValidPtr**

**Location:** `sdk/sdk.cpp`

**Problem:**
- Some functions use `IsValidPtr()`, others don't
- After `IsValidPtr()` check, subsequent operations aren't protected

---

### 12. **No Retry Logic for Failed Operations**

**Location:** Various SDK functions

**Problem:**
- If a function fails, it returns nullptr/0
- No retry mechanism
- No exponential backoff

---

### 13. **Static Timer Variables Not Thread-Safe**

**Location:** `sdk/sdk.cpp`

**Problem:**
```cpp
static int refreshTimer = 0;
static int g_MitaCycleRefreshTimer = 0;
```
- Multiple threads can modify simultaneously
- No atomic operations

---

## 🟢 LOW PRIORITY ISSUES

### 14. **Logging Not Comprehensive**

**Location:** `core/core.cpp`

**Problem:**
- Only logs SEH exceptions in main thread
- Doesn't log crashes in other threads
- No logging during shutdown sequence

---

### 15. **Sleep in Critical Path**

**Location:** `core/core.cpp`

**Problem:**
```cpp
std::this_thread::sleep_for(std::chrono::milliseconds(16));
```
- 16ms sleep in main loop
- Could cause lag

---

## RECOMMENDED FIXES PRIORITY

### Must Fix Immediately (Critical Stability):
1. ✅ Fix IL2CPP thread attachment - attach once, detach properly
2. ✅ Add mutex protection for static pointer caching
3. ✅ Wrap all raw memory access in SEH blocks
4. ✅ Fix hook address resolution
5. ✅ Validate render target recreation
6. ✅ Add ImGui IO validation

### Should Fix Soon (High Priority):
7. ✅ Improve exception handling - don't suppress, recover or disable feature
8. ✅ Add thread detachment
9. ✅ Optimize mutex usage - copy outside lock
10. ✅ Add comprehensive logging

### Nice to Have (Medium Priority):
11. ✅ Consistent pointer validation
12. ✅ Add retry logic
13. ✅ Use atomic operations for timers

---

## THREAD SAFETY ARCHITECTURE ISSUES

The fundamental problem is that the code doesn't have a clear threading model:

**Current State:**
- ❌ Main thread does everything (hooks, features, rendering mixed)
- ❌ No clear ownership of resources
- ❌ Shared state without proper synchronization

**Recommended Architecture:**
- ✅ Main thread: Feature updates, game state reading
- ✅ Render thread: Only rendering (reads game state atomically)
- ✅ Clear separation: Game state → Render state (copy)
- ✅ Thread-local caches with atomic updates

---

## MEMORY SAFETY ISSUES SUMMARY

| Issue | Count | Severity |
|-------|-------|----------|
| Raw memory offset access | 15+ | Critical |
| Missing pointer validation | 20+ | High |
| Race conditions | 10+ | Critical |
| Hardcoded addresses | 2 | Critical |
| Missing SEH protection | 5+ | High |

---

## RECOMMENDATION FOR IMMEDIATE ACTION

**Phase 1 (Critical Fixes):**
1. Implement proper IL2CPP thread attachment/detachment
2. Add mutex protection for all static globals
3. Fix hook address resolution
4. Wrap all memory access in SEH

**Phase 2 (Stability Improvements):**
1. Improve exception handling
2. Add comprehensive logging
3. Validate all device objects

**Phase 3 (Performance):**
1. Optimize mutex usage
2. Use atomic operations where appropriate
3. Reduce sleep times

---

## CONCLUSION

The cheat has significant thread safety and memory access issues that explain the random crashes. The good news is that most issues can be fixed with proper synchronization, validation, and error handling. The crash logging system is a good start but needs to be more comprehensive.
