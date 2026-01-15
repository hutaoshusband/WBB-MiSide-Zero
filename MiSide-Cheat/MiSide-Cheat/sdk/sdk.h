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

    struct Matrix4x4 {
        float m[16]; // Unity is column-major usually, but let's just treat as 16 floats
        
        static Matrix4x4 Multiply(const Matrix4x4& lhs, const Matrix4x4& rhs) {
            Matrix4x4 res;
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    float sum = 0;
                    for (int i = 0; i < 4; i++) {
                        // m[row + col*4] if column major? 
                        // Unity: m00(0), m10(1), m20(2), m30(3) make up first column.
                        // Accessing element at row r, col c: m[r + c*4]
                        sum += lhs.m[r + i * 4] * rhs.m[i + c * 4];
                    }
                    res.m[r + c * 4] = sum;
                }
            }
            return res;
        }
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
        void* GetMitaManager(); 
        void* GetMitaAnimator(); // New: Get Animator from MitaManager

        // Components
        void* GetPlayerCamera(); // From PlayerManager
        void* GetMainCamera();   // From Unity Static Property
        void* GetPlayerMovement(); // kiriMove
        void* GetPlayerMoveBasic(); // kiriMoveBasic
        
        Vector3 GetTransformPosition(void* transform);
        Vector3 GetPosition(void* gameObjectOrComponent); // Smart wrapper
        Vector3 GetBonePosition(void* animator, int boneId);
        void SetSpeed(void* movement, float speed);

        // Matrix
        Matrix4x4 GetViewMatrix(void* camera);
        Matrix4x4 GetProjectionMatrix(void* camera);

        // World to Screen
        Vector3 WorldToScreen(Vector3 worldPos);

        // State & Components
        int GetMitaState();
        int GetMitaMovementState();
        void* GetPlayerRigidbody();
        void* GetPlayerCollider();
        void SetRigidbodyKinematic(void* rb, bool enabled);
        void SetColliderEnabled(void* col, bool enabled);
    }
}
