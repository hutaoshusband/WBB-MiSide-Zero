#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <d3d11.h>
#include <imgui.h>

namespace sdk {

    struct Il2CppDomain;
    struct Il2CppThread;
    struct Il2CppImage;
    struct Il2CppClass;
    struct Il2CppMethod;
    struct Il2CppField;
    struct Il2CppType;
    struct Il2CppObject;
    struct Il2CppString;

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
        float m[16];

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

    bool Initialize();
    bool IsValidPtr(void* ptr);
    bool IsReady();
    bool AttachCurrentThread();
    
    Il2CppDomain* GetDomain();
    Il2CppThread* ThreadAttach(Il2CppDomain* domain);
    
    Il2CppClass* GetClass(const char* namespaceName, const char* className);
    Il2CppMethod* GetMethod(Il2CppClass* klass, const char* methodName, int argsCount);
    Il2CppField* GetField(Il2CppClass* klass, const char* fieldName);
    
    void* GetStaticFieldValue(Il2CppClass* klass, const char* fieldName);
    void* GetInstanceFieldValue(void* instance, const char* fieldName);
    
    const char* GetLastLog();

    void* RuntimeInvoke(Il2CppMethod* method, void* instance, void** params, void** exc);
    
    void* GetSystemType(Il2CppClass* klass);

    namespace game {
        void* FindObjectOfType(const char* className);

        void* GetPlayerManager();
        void* GetMitaManager(); 
        void* GetMitaAnimator(); 

        void* GetPlayerCamera();
        void* GetMainCamera();
        void* GetPlayerMovement();
        void* GetPlayerLook();
        void* GetPlayerCameraScript();

        void SetBehaviourEnabled(void* behaviour, bool enabled);
        
        Vector3 GetTransformPosition(void* transform);
        Vector3 GetPosition(void* gameObjectOrComponent); 
        Vector3 GetBonePosition(void* animator, int boneId);
        float GetSpeed(void* movement);
        void SetSpeed(void* movement, float speed);

        Matrix4x4 GetViewMatrix(void* camera);
        Matrix4x4 GetProjectionMatrix(void* camera);
        Vector3 WorldToScreen(Vector3 worldPos);

        int GetMitaState();
        int GetMitaMovementState();
        void* GetPlayerRigidbody();
        void* GetPlayerCollider();
        void SetRigidbodyKinematic(void* rb, bool enabled);
        void SetColliderEnabled(void* col, bool enabled);

        std::vector<void*> GetRenderers(void* gameObjectOrComponent);
        void* CreateMaterial(void* shader);
        void* FindShader(const char* shaderName);
        void* GetMaterial(void* renderer);
        void SetMaterial(void* renderer, void* material);
        void SetMaterialColor(void* material, float r, float g, float b, float a);
        void SetMaterialInt(void* material, const char* propName, int value);
        void SetMaterialFloat(void* material, const char* propName, float value);
        
        void SetMaterialZTestAlways(void* material);
        void SetMaterialGlow(void* material, float r, float g, float b, float intensity);
        
        void SetMaterialEmissionColor(void* material, float r, float g, float b, float a);
        void SetMaterialGlowStrength(void* material, float strength);
        
        enum BodyPart {
            BodyPart_None = 0,
            BodyPart_Head = 1,
            BodyPart_Body = 2,
            BodyPart_Legs = 3,
            BodyPart_Arms = 4
        };
        
        BodyPart GetRendererBodyPart(void* renderer);
        
        std::vector<void*> GetRenderersByBodyPart(void* gameObject, BodyPart part);

        void* GetMitaNavMeshAgent();
        void SetAnimatorApplyRootMotion(void* animator, bool apply);
        void SetAgentSpeed(void* agent, float speed);
        void SetAgentAcceleration(void* agent, float acceleration);
        float GetAgentSpeed(void* agent);
        
        void* GetMitaCycle();
        int GetOutfitCount();
        int GetCurrentOutfit();
        bool SetOutfit(int index);
        void CycleNextOutfit();
        void CyclePreviousOutfit();
        std::vector<std::string> GetOutfitNames();
        
        void SetGameObjectActive(void* gameObject, bool active);

        void* GetPlayerCameraObject();
        float GetCameraFOV(void* camera);
        void SetCameraFOV(void* camera, float fov);

        Vector3 GetCameraPosition(void* camera);
        void SetCameraPosition(void* camera, Vector3 pos);
        Quaternion GetCameraRotation(void* camera);
        Vector3 GetCameraRotationEuler(void* camera);
        void SetCameraRotation(void* camera, Quaternion rot);
        Vector3 GetCameraForward(void* camera);

        void* GetCameraTransform(void* camera);
        Vector3 GetTransformPositionFast(void* transform);
        void SetTransformPositionFast(void* transform, Vector3 pos);
        Quaternion GetTransformRotationFast(void* transform);
        void SetTransformRotationFast(void* transform, Quaternion rot);

        void* GetKiriMove();
        float GetJumpHeight(void* kiriMove);
        void SetJumpHeight(void* kiriMove, float height);

        void* GetPlayerFeetTransform();
        void SetPlayerPositionDirect(Vector3 pos);

        std::vector<void*> GetActiveTweens();
        std::vector<Vector3> GetTweenPathPoints(void* tween);

        Vector3 GetRigidbodyVelocity(void* rb);
        
        std::vector<void*> FindObjectsOfTypeAll(const char* className);
        const char* GetObjectName(void* object);
        const char* GetLayerName(int layer);
    }
}
