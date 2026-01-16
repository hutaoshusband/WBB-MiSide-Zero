#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <mutex>
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

        Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }
        Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }
        Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    };
    
    struct Vector2 {
        float x, y;
    };
    
    struct Quaternion {
        float x, y, z, w;
    };

    struct Matrix4x4 {
        float m[16]; // Column-major (Unity default)

        // Index operator for easy access (row, col)
        float& operator()(int row, int col) {
            return m[col * 4 + row];
        }
        const float& operator()(int row, int col) const {
            return m[col * 4 + row];
        }
        
        static Matrix4x4 Multiply(const Matrix4x4& lhs, const Matrix4x4& rhs) {
            Matrix4x4 res;
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    float sum = 0;
                    for (int i = 0; i < 4; i++) {
                        sum += lhs(r, i) * rhs(i, c);
                    }
                    res(r, c) = sum;
                }
            }
            return res;
        }

        static Matrix4x4 Transpose(const Matrix4x4& mat) {
            Matrix4x4 res;
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    res(r, c) = mat(c, r);
                }
            }
            return res;
        }
    };

    // ============================================================
    // API Wrappers
    // ============================================================
    bool Initialize();
    bool IsValidPtr(void* ptr);
    
    // Return true if SDK is ready to use
    bool IsReady();
    
    // Attach current thread to IL2CPP runtime (MUST be called before any IL2CPP calls from non-Unity threads!)
    // Returns true if already attached or attachment succeeded
    bool AttachCurrentThread();
    
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
        void* GetMitaAnimator(); 

        // Components
        void* GetPlayerCamera(); // From PlayerManager
        void* GetMainCamera();   // From Unity Static Property
        void* GetPlayerMovement(); // kiriMoveBasic
        
        Vector3 GetTransformPosition(void* transform);
        Vector3 GetPosition(void* gameObjectOrComponent); 
        Vector3 GetBonePosition(void* animator, int boneId);
        float GetSpeed(void* movement);
        void SetSpeed(void* movement, float speed);

        // Matrix & W2S
        Matrix4x4 GetViewMatrix(void* camera);
        Matrix4x4 GetProjectionMatrix(void* camera);
        Vector3 WorldToScreen(Vector3 worldPos); // Manual Matrix Implementation

        // State & Components
        int GetMitaState();
        int GetMitaMovementState();
        void* GetPlayerRigidbody();
        void* GetPlayerCollider();
        void SetRigidbodyKinematic(void* rb, bool enabled);
        void SetColliderEnabled(void* col, bool enabled);

        // Chams / Materials
        std::vector<void*> GetRenderers(void* gameObjectOrComponent);
        void* CreateMaterial(void* shader);
        void* FindShader(const char* shaderName);
        void* GetMaterial(void* renderer);
        void SetMaterial(void* renderer, void* material);
        void SetMaterialColor(void* material, float r, float g, float b, float a);
        void SetMaterialInt(void* material, const char* propName, int value);
        void SetMaterialFloat(void* material, const char* propName, float value);
        
        // Material helper to make it draw through walls (ZTest Always)
        void SetMaterialZTestAlways(void* material);
        void SetMaterialGlow(void* material, float r, float g, float b, float intensity);
        
        // Glow-specific material properties
        void SetMaterialEmissionColor(void* material, float r, float g, float b, float a);
        void SetMaterialGlowStrength(void* material, float strength);
        
        // Renderer filtering for partial body modulation
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

        // ============================================================
        // Mita Control / NavMeshAgent
        // ============================================================
        void* GetMitaNavMeshAgent();
        void SetAnimatorApplyRootMotion(void* animator, bool apply);
        void SetAgentSpeed(void* agent, float speed);
        void SetAgentAcceleration(void* agent, float acceleration);
        float GetAgentSpeed(void* agent);
        
        // ============================================================
        // Outfit Changer (based on MitaCycle/MitaCycler dump analysis)
        // ============================================================
        // MitaCycle class contains themeOptions (List<MitaCycler>) at offset 0x30
        // MitaCycler contains currentMita (GameObject) at offset 0x20
        // currentIndex at offset 0x80 tracks which outfit is active
        
        void* GetMitaCycle();                    // Find MitaCycle instance
        int GetOutfitCount();                    // Get number of available outfits  
        int GetCurrentOutfit();                  // Get current outfit index
        bool SetOutfit(int index);               // Set outfit by index
        void CycleNextOutfit();                  // Cycle to next outfit
        void CyclePreviousOutfit();              // Cycle to previous outfit
        std::vector<std::string> GetOutfitNames(); // Get list of outfit names
        
        // GameObject helpers
        void SetGameObjectActive(void* gameObject, bool active);

        // ============================================================
        // Player Modification
        // ============================================================
        void* GetPlayerCameraObject();
        float GetCameraFOV(void* camera);
        void SetCameraFOV(void* camera, float fov);
        void* GetKiriMove();
        float GetJumpHeight(void* kiriMove);
        void SetJumpHeight(void* kiriMove, float height);

        // ============================================================
        // DOTween / Path Prediction
        // ============================================================
        std::vector<void*> GetActiveTweens();
        std::vector<Vector3> GetTweenPathPoints(void* tween);

        // Physics Prediction
        Vector3 GetRigidbodyVelocity(void* rb);
        
        // Object Finding / Utils
        std::vector<void*> FindObjectsOfTypeAll(const char* className);
        const char* GetObjectName(void* object);
        const char* GetLayerName(int layer);
    }
}
