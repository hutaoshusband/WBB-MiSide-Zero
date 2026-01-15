#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <d3d11.h>
#include <imgui.h>

namespace sdk {

    // ============================================================
    // IL2CPP Types
    // ============================================================
    struct Il2CppDomain;
    struct Il2CppThread;
    struct Il2CppImage;
    struct Il2CppClass;
    struct Il2CppMethod;
    struct Il2CppField;
    struct Il2CppType;
    struct Il2CppObject;
    struct Il2CppString;

    // ============================================================
    // Unity Math
    // ============================================================
    struct Vector3 {
        float x, y, z;
        
        inline float Distance(const Vector3& other) const {
            float dx = x - other.x;
            float dy = y - other.y;
            float dz = z - other.z;
            return sqrt(dx*dx + dy*dy + dz*dz);
        }
    };
    
    struct Vector2 {
        float x, y;
    };
    
    struct Quaternion {
        float x, y, z, w;
    };

    // ============================================================
    // API Wrappers
    // ============================================================
    bool Initialize();
    
    Il2CppDomain* GetDomain();
    Il2CppThread* ThreadAttach(Il2CppDomain* domain);
    
    // Class Resolution
    Il2CppClass* GetClass(const char* namespaceName, const char* className);
    Il2CppMethod* GetMethod(Il2CppClass* klass, const char* methodName, int argsCount);
    Il2CppField* GetField(Il2CppClass* klass, const char* fieldName);
    
    // Field Access
    void* GetStaticFieldValue(Il2CppClass* klass, const char* fieldName);
    void* GetInstanceFieldValue(void* instance, const char* fieldName);
    
    // Debug
    const char* GetLastLog();

    // Method Invocation
    void* RuntimeInvoke(Il2CppMethod* method, void* instance, void** params, void** exc);
    
    // Type System
    void* GetSystemType(Il2CppClass* klass);

    // ============================================================
    // Game Specific Helpers
    // ============================================================
    namespace game {
        // General
        void* FindObjectOfType(const char* className);

        // Managers
        void* GetPlayerManager();
        void* GetMitaManager(); // Try to find
        
        // Components
        void* GetPlayerCamera(); // From PlayerManager
        Vector3 GetTransformPosition(void* transform);
        Vector3 GetPosition(void* gameObjectOrComponent); // Smart wrapper
        
        // World to Screen
        Vector3 WorldToScreen(Vector3 worldPos);
    }
}
