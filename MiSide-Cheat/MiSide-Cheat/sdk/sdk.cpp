#include "sdk.h"
#include <iostream>
#include <windows.h>
#include <fstream>
#include <shlobj.h>

namespace sdk {
    
    static std::string g_LastLog = "No logs yet.";
    void Log(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        g_LastLog = buffer;

        // Log to file
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
            std::string logPath = std::string(path) + "\\wbb_miside_log.txt";
            std::ofstream logFile(logPath, std::ios_base::app);
            if (logFile.is_open()) {
                logFile << buffer << std::endl;
            }
        }
    }

    const char* GetLastLog() {
        return g_LastLog.c_str();
    }

    // ============================================================
    // IL2CPP Function Pointers
    // ============================================================
    typedef Il2CppDomain* (*t_il2cpp_domain_get)();
    typedef Il2CppThread* (*t_il2cpp_thread_attach)(Il2CppDomain*);
    typedef void* (*t_il2cpp_resolve_icall)(const char*);
    typedef Il2CppImage* (*t_il2cpp_assembly_get_image)(void* assembly);
    typedef void** (*t_il2cpp_domain_get_assemblies)(void* domain, size_t* size);
    typedef Il2CppClass* (*t_il2cpp_class_from_name)(Il2CppImage*, const char*, const char*);
    typedef Il2CppMethod* (*t_il2cpp_class_get_method_from_name)(Il2CppClass*, const char*, int);
    typedef Il2CppField* (*t_il2cpp_class_get_field_from_name)(Il2CppClass*, const char*);
    typedef void (*t_il2cpp_field_static_get_value)(Il2CppField*, void*);
    typedef void (*t_il2cpp_field_get_value)(void*, Il2CppField*, void*);
    typedef void* (*t_il2cpp_runtime_invoke)(Il2CppMethod*, void*, void**, void**);
    typedef const char* (*t_il2cpp_image_get_name)(Il2CppImage*);
    typedef Il2CppType* (*t_il2cpp_class_get_type)(Il2CppClass*);
    typedef void* (*t_il2cpp_type_get_object)(Il2CppType*);

    static t_il2cpp_domain_get il2cpp_domain_get = nullptr;
    static t_il2cpp_thread_attach il2cpp_thread_attach = nullptr;
    static t_il2cpp_domain_get_assemblies il2cpp_domain_get_assemblies = nullptr;
    static t_il2cpp_assembly_get_image il2cpp_assembly_get_image = nullptr;
    static t_il2cpp_class_from_name il2cpp_class_from_name = nullptr;
    static t_il2cpp_class_get_method_from_name il2cpp_class_get_method_from_name = nullptr;
    static t_il2cpp_class_get_field_from_name il2cpp_class_get_field_from_name = nullptr;
    static t_il2cpp_field_static_get_value il2cpp_field_static_get_value = nullptr;
    static t_il2cpp_field_get_value il2cpp_field_get_value = nullptr;
    static t_il2cpp_runtime_invoke il2cpp_runtime_invoke = nullptr;
    static t_il2cpp_image_get_name il2cpp_image_get_name = nullptr;
    static t_il2cpp_class_get_type il2cpp_class_get_type = nullptr;
    static t_il2cpp_type_get_object il2cpp_type_get_object = nullptr;

    HMODULE g_GameAssembly = nullptr;
    bool g_Initialized = false;

    bool Initialize() {
        if (g_Initialized) return true;

        static t_il2cpp_resolve_icall il2cpp_resolve_icall = nullptr;
        
        g_GameAssembly = GetModuleHandleA("GameAssembly.dll");
        if (!g_GameAssembly) return false;

        #define LOAD_PROC(name) name = (t_##name)GetProcAddress(g_GameAssembly, #name); if(!name) return false;

        LOAD_PROC(il2cpp_domain_get);
        LOAD_PROC(il2cpp_thread_attach);
        LOAD_PROC(il2cpp_resolve_icall);
        LOAD_PROC(il2cpp_domain_get_assemblies);
        LOAD_PROC(il2cpp_assembly_get_image);
        LOAD_PROC(il2cpp_class_from_name);
        LOAD_PROC(il2cpp_class_get_method_from_name);
        LOAD_PROC(il2cpp_class_get_field_from_name);
        LOAD_PROC(il2cpp_field_static_get_value);
        LOAD_PROC(il2cpp_field_get_value);
        LOAD_PROC(il2cpp_runtime_invoke);
        LOAD_PROC(il2cpp_image_get_name);
        LOAD_PROC(il2cpp_class_get_type);
        LOAD_PROC(il2cpp_type_get_object);

        // Attach thread
        il2cpp_thread_attach(il2cpp_domain_get());

        g_Initialized = true;
        return true;
    }

    Il2CppDomain* GetDomain() {
        return il2cpp_domain_get ? il2cpp_domain_get() : nullptr;
    }

    Il2CppThread* ThreadAttach(Il2CppDomain* domain) {
        return il2cpp_thread_attach ? il2cpp_thread_attach(domain) : nullptr;
    }

    Il2CppClass* GetClass(const char* namespaceName, const char* className) {
        if (!g_Initialized) return nullptr;

        size_t size = 0;
        void** assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);

        for (size_t i = 0; i < size; ++i) {
            Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
            if (image) {
                Il2CppClass* klass = il2cpp_class_from_name(image, namespaceName, className);
                if (klass) return klass;
            }
        }
        return nullptr;
    }

    Il2CppMethod* GetMethod(Il2CppClass* klass, const char* methodName, int argsCount) {
        if (!klass) return nullptr;
        return il2cpp_class_get_method_from_name(klass, methodName, argsCount);
    }
    
    Il2CppField* GetField(Il2CppClass* klass, const char* fieldName) {
        if (!klass) return nullptr;
        return il2cpp_class_get_field_from_name(klass, fieldName);
    }

    void* GetStaticFieldValue(Il2CppClass* klass, const char* fieldName) {
        if (!klass) return nullptr;
        Il2CppField* field = GetField(klass, fieldName);
        if (!field) return nullptr;
        
        void* value = nullptr;
        il2cpp_field_static_get_value(field, &value);
        return value; // For pointer types, this is the value
    }

    void* GetInstanceFieldValue(void* instance, const char* fieldName) {
        // Find field in class (we assume we know the class or query it)
        // For simplicity allow passing class, or just object (need object_get_class)
        // Here we will assume the caller knows exactly what they are doing or we improve this later
        return nullptr; // Need object class support
    }
    // Improved version requiring class
    void* GetValue(void* instance, Il2CppClass* klass, const char* fieldName) {
         if (!instance || !klass) return nullptr;
         Il2CppField* field = GetField(klass, fieldName);
         if (!field) return nullptr;
         
         void* value = nullptr;
         il2cpp_field_get_value(instance, field, &value);
         return value;
    }

    // Instance field wrapper with manual class lookup (expensive, but useful)
    // We'll skip for now and use specific getters

    void* RuntimeInvoke(Il2CppMethod* method, void* instance, void** params, void** exc) {
        if (!method) return nullptr;
        return il2cpp_runtime_invoke(method, instance, params, exc);
    }

    void* GetSystemType(Il2CppClass* klass) {
        if (!klass || !il2cpp_class_get_type || !il2cpp_type_get_object) return nullptr;
        Il2CppType* type = il2cpp_class_get_type(klass);
        if (!type) return nullptr;
        return il2cpp_type_get_object(type);
    }

    namespace game {
        void* g_PlayerManagerClass = nullptr;
        void* g_MitaManagerClass = nullptr;
        void* g_CameraClass = nullptr;
        void* g_ComponentClass = nullptr;
        void* g_TransformClass = nullptr;

        void* GetPlayerManager() {
            if (!g_PlayerManagerClass) {
                g_PlayerManagerClass = GetClass("", "PlayerManager");
            }
            if (!g_PlayerManagerClass) return nullptr;

            // Get static instance
            return GetStaticFieldValue((Il2CppClass*)g_PlayerManagerClass, "instance");
        }

        void* GetPlayerCamera() {
            void* pm = GetPlayerManager();
            if (!pm) return nullptr;

            // Get 'playerCam' field
            static Il2CppField* camField = nullptr;
            if (!camField) camField = GetField((Il2CppClass*)g_PlayerManagerClass, "playerCam");
            if (!camField) return nullptr;

            void* cam = nullptr;
            il2cpp_field_get_value(pm, camField, &cam);
            return cam;
        }

        void* GetPlayerMovement() {
            void* pm = GetPlayerManager();
            if (!pm) return nullptr;
            static Il2CppField* moveField = GetField((Il2CppClass*)GetClass("", "PlayerManager"), "move");
            if (!moveField) return nullptr;
            void* move = nullptr;
            il2cpp_field_get_value(pm, moveField, &move);
            return move;
        }

        void* GetPlayerMoveBasic() {
            void* pm = GetPlayerManager();
            if (!pm) return nullptr;
            // Some versions might use a different field, but in dump it was at 0x58
            static Il2CppField* moveField = GetField((Il2CppClass*)GetClass("", "PlayerManager"), "move");
            if (!moveField) return nullptr;
            void* move = nullptr;
            il2cpp_field_get_value(pm, moveField, &move);
            return move;
        }

        void* GetMainCamera() {
            if (!g_CameraClass) g_CameraClass = GetClass("UnityEngine", "Camera");
            if (!g_CameraClass) return nullptr;

            static Il2CppMethod* getMain = nullptr;
            if (!getMain) getMain = GetMethod((Il2CppClass*)g_CameraClass, "get_main", 0);
            if (!getMain) return nullptr;

            void* cam = RuntimeInvoke(getMain, nullptr, nullptr, nullptr);
            return cam;
        }

        void* FindObjectOfType(const char* className) {
             Il2CppClass* targetClass = GetClass("", className);
             if (!targetClass) {
                 Log("FindObjectOfType: Class '%s' not found.", className);
                 return nullptr;
             }

             void* systemType = GetSystemType(targetClass);
             if (!systemType) {
                 Log("FindObjectOfType: Failed to get System.Type for '%s'.", className);
                 return nullptr;
             }

             // Use RVA 0x19359E0 for UnityEngine.Object.FindObjectOfType(Type)
             // RVA: 0x19359E0 Offset: 0x1934DE0 VA: 0x1819359E0
             static uintptr_t rva = 0x19359E0;
             static void* (*FindObjectOfTypeFunc)(void*) = nullptr;
             
             if (!FindObjectOfTypeFunc) {
                 FindObjectOfTypeFunc = (void* (*)(void*))((uintptr_t)g_GameAssembly + rva);
                 Log("FindObjectOfType: Resolved func at %p (Base: %p + RVA: %p)", FindObjectOfTypeFunc, g_GameAssembly, (void*)rva);
             }

             void* result = FindObjectOfTypeFunc(systemType);
             
             if (result) Log("FindObjectOfType: Found instance of '%s' at %p", className, result);
             else Log("FindObjectOfType: No instance of '%s' found (result is null).", className);
             
             return result;
        }

        void* GetMitaManager() {
             // Try to find MitaManager via FindObjectOfType
             // This is likely more reliable than assuming a static instance if it doesn't have one
             static void* mitaCached = nullptr;
             
             // Refresh every so often or if null? For now just find if null.
             if (!mitaCached) {
                 mitaCached = FindObjectOfType("MitaManager");
             }
             return mitaCached;
        }

        void* GetMitaAnimator() {
            void* mita = GetMitaManager();
            if (!mita) return nullptr;

            // MitaManager has 'public Animator animManager;' at offset 0x30 according to dump
            // Let's resolve by name to be safe
            static Il2CppClass* mitaClass = nullptr;
            if (!mitaClass) mitaClass = GetClass("", "MitaManager");
            if (!mitaClass) return nullptr;

            static Il2CppField* animField = nullptr;
            if (!animField) animField = GetField(mitaClass, "animManager");
            if (!animField) return nullptr;

            void* animator = nullptr;
            il2cpp_field_get_value(mita, animField, &animator);
            return animator;
        }

        int GetMitaState() {
            void* mita = GetMitaManager();
            if (!mita) return 0;

            static Il2CppClass* mitaClass = nullptr;
            if (!mitaClass) mitaClass = GetClass("", "MitaManager");
            if (!mitaClass) return 0;

            static Il2CppField* stateField = nullptr;
            if (!stateField) stateField = GetField(mitaClass, "state");
            if (!stateField) return 0;

            int state = 0;
            il2cpp_field_get_value(mita, stateField, &state);
            return state;
        }

        int GetMitaMovementState() {
            void* mita = GetMitaManager();
            if (!mita) return 0;

            static Il2CppClass* mitaClass = nullptr;
            if (!mitaClass) mitaClass = GetClass("", "MitaManager");
            if (!mitaClass) return 0;

            static Il2CppField* moveField = nullptr;
            if (!moveField) moveField = GetField(mitaClass, "move");
            if (!moveField) return 0;

            void* mitaMove = nullptr;
            il2cpp_field_get_value(mita, moveField, &mitaMove);
            if (!mitaMove) return 0;

            static Il2CppClass* moveClass = nullptr;
            if (!moveClass) moveClass = GetClass("", "MitaMove");
            if (!moveClass) return 0;

            static Il2CppField* movStateField = nullptr;
            if (!movStateField) movStateField = GetField(moveClass, "movementState");
            if (!movStateField) return 0;

            int state = 0;
            il2cpp_field_get_value(mitaMove, movStateField, &state);
            return state;
        }

        void SetSpeed(void* movement, float speed) {
            if (!movement) return;
            
            // In the dump, PlayerManager.move is kiriMoveBasic (TypeDefIndex: 4197).
            // kiriMoveBasic: walkSpeed at 0x1C.
            // kiriMove: walkSpeed at 0x18.
            // We'll write to both but avoiding 0x18 if we suspect it's Basic (to not break canMove).
            
            // Check if 0x1C looks like a speed float (around 1.0-20.0)
            float speed1C = *(float*)((uintptr_t)movement + 0x1C);
            if (speed1C > 0.1f && speed1C < 50.0f) {
                *(float*)((uintptr_t)movement + 0x1C) = speed;
                // Also ensure canMove is on
                *(bool*)((uintptr_t)movement + 0x18) = true;
            } else {
                // If 0x1C is not speed, maybe it's 0x18
                *(float*)((uintptr_t)movement + 0x18) = speed;
            }
        }

        void* GetPlayerRigidbody() {
            void* pm = GetPlayerManager();
            if (!pm) return nullptr;

            static Il2CppClass* pmClass = nullptr;
            if (!pmClass) pmClass = GetClass("", "PlayerManager");
            if (!pmClass) return nullptr;

            static Il2CppField* rbField = nullptr;
            if (!rbField) rbField = GetField(pmClass, "rb");
            if (!rbField) return nullptr;

            void* rb = nullptr;
            il2cpp_field_get_value(pm, rbField, &rb);
            return rb;
        }

        void* GetPlayerCollider() {
            void* pm = GetPlayerManager();
            if (!pm) return nullptr;

            static Il2CppClass* pmClass = nullptr;
            if (!pmClass) pmClass = GetClass("", "PlayerManager");
            if (!pmClass) return nullptr;

            static Il2CppField* colField = nullptr;
            if (!colField) colField = GetField(pmClass, "col");
            if (!colField) return nullptr;

            void* col = nullptr;
            il2cpp_field_get_value(pm, colField, &col);
            return col;
        }

        typedef void* (*t_il2cpp_resolve_icall)(const char*);
        static t_il2cpp_resolve_icall resolve_icall = nullptr;

        void SetRigidbodyKinematic(void* rb, bool enabled) {
            if (!rb) return;
            typedef void(__stdcall* fnSetKinematic)(void*, bool);
            static fnSetKinematic s_fn = nullptr;
            
            if (!s_fn) {
                if (!resolve_icall) resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
                if (resolve_icall) s_fn = (fnSetKinematic)resolve_icall("UnityEngine.Rigidbody::set_isKinematic(System.Boolean)");
            }
            
            if (s_fn) s_fn(rb, enabled);
            else {
                // Fallback RVA: 0x19927F0
                static void(__fastcall* rva_fn)(void*, bool) = (void(__fastcall*)(void*, bool))((uintptr_t)GetModuleHandleA("GameAssembly.dll") + 0x19927F0);
                rva_fn(rb, enabled);
            }
        }

        void SetColliderEnabled(void* col, bool enabled) {
            if (!col) return;
            typedef void(__stdcall* fnSetEnabled)(void*, bool);
            static fnSetEnabled s_fn = nullptr;
            
            if (!s_fn) {
                if (!resolve_icall) resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
                if (resolve_icall) s_fn = (fnSetEnabled)resolve_icall("UnityEngine.Collider::set_enabled(System.Boolean)");
            }
            
            if (s_fn) s_fn(col, enabled);
            else {
                // Fallback RVA: 0x198D710
                static void(__fastcall* rva_fn)(void*, bool) = (void(__fastcall*)(void*, bool))((uintptr_t)GetModuleHandleA("GameAssembly.dll") + 0x198D710);
                rva_fn(col, enabled);
            }
        }

        Vector3 GetTransformPosition(void* transform) {
            if (!transform) return {0,0,0};
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");
            
            static Il2CppMethod* getPos = nullptr;
            if (!getPos) getPos = GetMethod((Il2CppClass*)g_TransformClass, "get_position", 0);
            if (!getPos) return {0,0,0};

            Vector3 res = {0,0,0};
            void* ret = RuntimeInvoke(getPos, transform, nullptr, nullptr);
            if (ret) {
                 res = *(Vector3*)((char*)ret + 0x10); // Unbox
            }
            return res;
        }

        Vector3 GetPosition(void* component) {
            if (!component) return {0,0,0};
            
            // If it IS a transform, use it directly
            // But we don't have easy type check. Assume it's a Component.
            
            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
            
            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);
            
            if (!getTrans) return {0,0,0};
            
            void* transform = RuntimeInvoke(getTrans, component, nullptr, nullptr);
            return GetTransformPosition(transform);
        }

        Vector3 GetBonePosition(void* animator, int boneId) {
            if (!animator) return {0,0,0};
            static Il2CppClass* animatorClass = GetClass("UnityEngine", "Animator");
            if (!animatorClass) return {0,0,0};

            static Il2CppMethod* getBoneTransform = GetMethod(animatorClass, "GetBoneTransform", 1);
            if (!getBoneTransform) return {0,0,0};

            void* args[1] = { &boneId }; // HumanBodyBones enum is int-sized
            void* transform = RuntimeInvoke(getBoneTransform, animator, args, nullptr);
            
            return GetTransformPosition(transform);
        }

        // Direct RVA function pointers for matrix getters (bypasses RuntimeInvoke instability)
        // These _Injected methods write directly to output parameter
        typedef void(__fastcall* t_get_worldToCameraMatrix_Injected)(void* camera, Matrix4x4* ret);
        typedef void(__fastcall* t_get_projectionMatrix_Injected)(void* camera, Matrix4x4* ret);
        
        static t_get_worldToCameraMatrix_Injected get_worldToCameraMatrix_Injected = nullptr;
        static t_get_projectionMatrix_Injected get_projectionMatrix_Injected = nullptr;
        
        Matrix4x4 GetViewMatrix(void* camera) {
            if (!camera) return {0};
            
            // Initialize RVA function pointer
            if (!get_worldToCameraMatrix_Injected) {
                get_worldToCameraMatrix_Injected = (t_get_worldToCameraMatrix_Injected)((uintptr_t)g_GameAssembly + 0x190E6C0);
            }
            
            Matrix4x4 result = {0};
            get_worldToCameraMatrix_Injected(camera, &result);
            return result;
        }

        Matrix4x4 GetProjectionMatrix(void* camera) {
            if (!camera) return {0};
            
            // Initialize RVA function pointer
            if (!get_projectionMatrix_Injected) {
                get_projectionMatrix_Injected = (t_get_projectionMatrix_Injected)((uintptr_t)g_GameAssembly + 0x190E370);
            }
            
            Matrix4x4 result = {0};
            get_projectionMatrix_Injected(camera, &result);
            return result;
        }

        // Cached VP matrix - updated less frequently to reduce jitter
        static Matrix4x4 g_cachedVP = {0};
        static void* g_cachedCam = nullptr;
        static DWORD g_lastMatrixUpdate = 0;
        static bool g_vpValid = false;
        
        // Per-entity smoothed screen positions
        struct SmoothedScreenPos {
            Vector3 smoothedPos;
            Vector3 lastWorldPos;
            bool initialized;
        };
        static SmoothedScreenPos g_mitaSmoothed = {{0,0,0}, {0,0,0}, false};
        
        Vector3 WorldToScreen(Vector3 worldPos) {
            void* cam = GetMainCamera();
            if (!cam) cam = GetPlayerCamera();
            if (!cam) return {-10000, -10000, -1};

            static Il2CppMethod* w2sMethod = nullptr;
            if (!w2sMethod) {
                Il2CppClass* camClass = GetClass("UnityEngine", "Camera");
                w2sMethod = GetMethod(camClass, "WorldToScreenPoint", 1);
            }
            
            if (!w2sMethod) return {-10000, -10000, -1};

            void* params[1] = { &worldPos };
            void* ret = RuntimeInvoke(w2sMethod, cam, params, nullptr);
            if (!ret) return {-10000, -10000, -1};

            Vector3 res = *(Vector3*)((char*)ret + 0x10);
            
            // Flip Y for ImGui
            ImGuiIO& io = ImGui::GetIO();
            res.y = io.DisplaySize.y - res.y;
            
            return res;
        }


    }
}
