#include "sdk.h"
#include <iostream>
#include <windows.h>
#include <fstream>
#include <shlobj.h>
#include <string>
#include <algorithm>
#include <cctype>

namespace sdk {
    
    static std::string g_LastLog = "No logs yet.";
    void Log(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        g_LastLog = buffer;
    }

    const char* GetLastLog() {
        return g_LastLog.c_str();
    }

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
    typedef void (*t_il2cpp_field_set_value)(void*, Il2CppField*, void*);
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
    static t_il2cpp_field_set_value il2cpp_field_set_value = nullptr;
    static t_il2cpp_runtime_invoke il2cpp_runtime_invoke = nullptr;
    static t_il2cpp_image_get_name il2cpp_image_get_name = nullptr;
    static t_il2cpp_class_get_type il2cpp_class_get_type = nullptr;
    static t_il2cpp_type_get_object il2cpp_type_get_object = nullptr;
    static t_il2cpp_resolve_icall il2cpp_resolve_icall = nullptr;

    HMODULE g_GameAssembly = nullptr;
    bool g_Initialized = false;

    bool IsValidPtr(void* ptr) {
        if (!ptr || (uintptr_t)ptr < 0x10000 || (uintptr_t)ptr > 0x7FFFFFFFFFFF) return false;
        
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
                return true;
            }
        }
        return false;
    }
    
    static thread_local bool t_ThreadAttached = false;
    static thread_local Il2CppThread* t_AttachedThread = nullptr;
    
    static std::mutex g_SDKMutex;
    
    template<typename T>
    bool SafeRead(void* address, T& outValue) {
        if (!IsValidPtr(address)) return false;
        __try {
            outValue = *reinterpret_cast<T*>(address);
            return true;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    bool Initialize() {
        if (g_Initialized) return true;

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
        LOAD_PROC(il2cpp_field_set_value);
        LOAD_PROC(il2cpp_runtime_invoke);
        LOAD_PROC(il2cpp_image_get_name);
        LOAD_PROC(il2cpp_class_get_type);
        LOAD_PROC(il2cpp_type_get_object);

        Il2CppDomain* domain = il2cpp_domain_get();
        if (domain) {
            t_AttachedThread = il2cpp_thread_attach(domain);
            t_ThreadAttached = (t_AttachedThread != nullptr);
        }

        g_Initialized = true;
        return true;
    }
    
    bool IsReady() {
        return g_Initialized && g_GameAssembly != nullptr;
    }
    
    bool AttachCurrentThread() {
        if (t_ThreadAttached && t_AttachedThread) {
            return true;
        }
        
        if (!g_Initialized || !il2cpp_thread_attach || !il2cpp_domain_get) {
            return false;
        }
        
        Il2CppDomain* domain = il2cpp_domain_get();
        if (!domain) {
            return false;
        }
        
        t_AttachedThread = il2cpp_thread_attach(domain);
        t_ThreadAttached = (t_AttachedThread != nullptr);
        
        return t_ThreadAttached;
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
        return value; 
    }

    void* GetInstanceFieldValue(void* instance, const char* fieldName) {
        return nullptr; 
    }
   
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
        void* g_RendererClass = nullptr;
        void* g_ShaderClass = nullptr;
        void* g_MaterialClass = nullptr;

        void* GetPlayerManager() {
            if (!g_PlayerManagerClass) {
                g_PlayerManagerClass = GetClass("", "PlayerManager");
            }
            if (!g_PlayerManagerClass) return nullptr;
            return GetStaticFieldValue((Il2CppClass*)g_PlayerManagerClass, "instance");
        }

        void* GetPlayerCamera() {
            void* pm = GetPlayerManager();
            if (!pm) return nullptr;
            
            static Il2CppField* camField = nullptr;
            if (!camField) camField = GetField((Il2CppClass*)g_PlayerManagerClass, "playerCam");
            if (!camField) return nullptr;
            
            void* cam = nullptr;
            il2cpp_field_get_value(pm, camField, &cam);
            return cam;
        }

        static void GetFieldValueInternal(void* instance, Il2CppField* field, void** out) {
            __try {
                il2cpp_field_get_value(instance, field, out);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                *out = nullptr;
            }
        }

        void* GetPlayerMovement() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) {
                Log("GetPlayerMovement: PlayerManager is null or invalid");
                return nullptr;
            }
            
            if (!g_PlayerManagerClass) g_PlayerManagerClass = GetClass("", "PlayerManager");
            if (!g_PlayerManagerClass) {
                Log("GetPlayerMovement: PlayerManager class not found");
                return nullptr;
            }

            Il2CppField* moveField = GetField((Il2CppClass*)g_PlayerManagerClass, "move");
            if (!moveField) {
                Log("GetPlayerMovement: 'move' field not found in PlayerManager");
                return nullptr;
            }
            
            void* move = nullptr;
            GetFieldValueInternal(pm, moveField, &move);
            if (!move) {
                Log("GetPlayerMovement: 'move' field value is null");
            }
            return move;
        }

        void* GetMainCamera() {
            if (!g_CameraClass) g_CameraClass = GetClass("UnityEngine", "Camera");
            if (!g_CameraClass) return nullptr;
            static Il2CppMethod* getMain = nullptr;
            if (!getMain) getMain = GetMethod((Il2CppClass*)g_CameraClass, "get_main", 0);
            return RuntimeInvoke(getMain, nullptr, nullptr, nullptr);
        }

        void* FindObjectOfType(const char* className) {
             Il2CppClass* targetClass = GetClass("", className);
             if (!targetClass) return nullptr;

             void* systemType = GetSystemType(targetClass);
             if (!systemType) return nullptr;

             static uintptr_t rva = 0x19359E0;
             static void* (*FindObjectOfTypeFunc)(void*) = nullptr;
             
             if (!FindObjectOfTypeFunc) {
                 FindObjectOfTypeFunc = (void* (*)(void*))((uintptr_t)g_GameAssembly + rva);
             }

             return FindObjectOfTypeFunc(systemType);
        }

        void* GetMitaManager() {
             static void* mitaCached = nullptr;
             static int refreshTimer = 0;
             
             if (refreshTimer <= 0 || !mitaCached) {
                 mitaCached = FindObjectOfType("MitaManager");
                 refreshTimer = 60; // Refresh every ~1 second (assuming 60fps)
             } else {
                 refreshTimer--;
             }
             return mitaCached;
        }

        void* GetMitaAnimator() {
            void* mita = GetMitaManager();
            if (!mita) return nullptr;
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

        float GetSpeed(void* movement) {
            if (!IsValidPtr(movement)) return 0.0f;
            __try {
                return *(float*)((uintptr_t)movement + 0x1C);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return 0.0f;
            }
        }

        void SetSpeed(void* movement, float speed) {
            if (!IsValidPtr(movement)) return;
            __try {
                *(float*)((uintptr_t)movement + 0x1C) = speed;
                *(bool*)((uintptr_t)movement + 0x18) = true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        void* GetPlayerRigidbody() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) {
                Log("GetPlayerRigidbody: PlayerManager is null or invalid");
                return nullptr;
            }
            
            if (!g_PlayerManagerClass) g_PlayerManagerClass = GetClass("", "PlayerManager");
            if (!g_PlayerManagerClass) {
                Log("GetPlayerRigidbody: PlayerManager class not found");
                return nullptr;
            }

            Il2CppField* rbField = GetField((Il2CppClass*)g_PlayerManagerClass, "rb");
            if (!rbField) {
                Log("GetPlayerRigidbody: 'rb' field not found");
                return nullptr;
            }

            void* rb = nullptr;
            GetFieldValueInternal(pm, rbField, &rb);
            if (!rb) {
                Log("GetPlayerRigidbody: 'rb' field value is null");
            }
            return rb;
        }

        void* GetPlayerCollider() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) {
                Log("GetPlayerCollider: PlayerManager is null or invalid");
                return nullptr;
            }
            
            if (!g_PlayerManagerClass) g_PlayerManagerClass = GetClass("", "PlayerManager");
            if (!g_PlayerManagerClass) {
                Log("GetPlayerCollider: PlayerManager class not found");
                return nullptr;
            }

            Il2CppField* colField = GetField((Il2CppClass*)g_PlayerManagerClass, "col");
            if (!colField) {
                Log("GetPlayerCollider: 'col' field not found");
                return nullptr;
            }

            void* col = nullptr;
            GetFieldValueInternal(pm, colField, &col);
            if (!col) {
                Log("GetPlayerCollider: 'col' field value is null");
            }
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
        }

        Vector3 GetTransformPosition(void* transform) {
            if (!transform) return {0,0,0};
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");
            
            static Il2CppMethod* getPos = nullptr;
            if (!getPos) getPos = GetMethod((Il2CppClass*)g_TransformClass, "get_position", 0);

            Vector3 res = {0,0,0};
            void* ret = RuntimeInvoke(getPos, transform, nullptr, nullptr);
            if (ret) {
                 res = *(Vector3*)((char*)ret + 0x10); 
            }
            return res;
        }

        Vector3 GetPosition(void* component) {
            if (!component) return {0,0,0};
            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
            
            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);
            
            void* transform = RuntimeInvoke(getTrans, component, nullptr, nullptr);
            return GetTransformPosition(transform);
        }

        Vector3 GetBonePosition(void* animator, int boneId) {
            if (!animator) return {0,0,0};
            static Il2CppClass* animatorClass = nullptr;
            if (!animatorClass) animatorClass = GetClass("UnityEngine", "Animator");
            if (!animatorClass) return {0,0,0};
            
            static Il2CppMethod* getBoneTransform = nullptr;
            if (!getBoneTransform) getBoneTransform = GetMethod(animatorClass, "GetBoneTransform", 1);
            if (!getBoneTransform) return {0,0,0};

            void* args[1] = { &boneId };
            void* transform = RuntimeInvoke(getBoneTransform, animator, args, nullptr);
            if (!transform) return {0,0,0};
            return GetTransformPosition(transform);
        }

        
        typedef void(__fastcall* t_get_worldToCameraMatrix_Injected)(void* camera, Matrix4x4* ret);
        typedef void(__fastcall* t_get_projectionMatrix_Injected)(void* camera, Matrix4x4* ret);
        
        static t_get_worldToCameraMatrix_Injected get_worldToCameraMatrix_Injected = nullptr;
        static t_get_projectionMatrix_Injected get_projectionMatrix_Injected = nullptr;
        
        Matrix4x4 GetViewMatrix(void* camera) {
            if (!camera) return {0};
            if (!get_worldToCameraMatrix_Injected) {
                get_worldToCameraMatrix_Injected = (t_get_worldToCameraMatrix_Injected)((uintptr_t)g_GameAssembly + 0x190E6C0);
            }
            Matrix4x4 result = {0};
            get_worldToCameraMatrix_Injected(camera, &result);
            return result;
        }

        Matrix4x4 GetProjectionMatrix(void* camera) {
            if (!camera) return {0};
            if (!get_projectionMatrix_Injected) {
                get_projectionMatrix_Injected = (t_get_projectionMatrix_Injected)((uintptr_t)g_GameAssembly + 0x190E370);
            }
            Matrix4x4 result = {0};
            get_projectionMatrix_Injected(camera, &result);
            return result;
        }

        Vector3 WorldToScreen(Vector3 worldPos) {
            void* playerCam = GetPlayerCamera();
            void* mainCam = GetMainCamera();
            
            void* cam = playerCam ? playerCam : mainCam;
            
            if (!cam || !IsValidPtr(cam)) return {-10000, -10000, -1};

            Matrix4x4 view = {0};
            Matrix4x4 proj = {0};
            
            __try {
                view = GetViewMatrix(cam);
                proj = GetProjectionMatrix(cam);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                return {-10000, -10000, -1};
            }

            Matrix4x4 vp = Matrix4x4::Multiply(proj, view);

            float x = worldPos.x * vp(0, 0) + worldPos.y * vp(0, 1) + worldPos.z * vp(0, 2) + vp(0, 3);
            float y = worldPos.x * vp(1, 0) + worldPos.y * vp(1, 1) + worldPos.z * vp(1, 2) + vp(1, 3);
            float z = worldPos.x * vp(2, 0) + worldPos.y * vp(2, 1) + worldPos.z * vp(2, 2) + vp(2, 3);
            float w = worldPos.x * vp(3, 0) + worldPos.y * vp(3, 1) + worldPos.z * vp(3, 2) + vp(3, 3);

            if (w < 0.01f) return {-10000, -10000, -1};

            float invW = 1.0f / w;
            float ndcX = x * invW;
            float ndcY = y * invW;
            float ndcZ = z * invW;

            __try {
                ImGuiIO& io = ImGui::GetIO();
                
                if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f) {
                    return {-10000, -10000, -1};
                }
                
                Vector3 screen = {
                    (ndcX + 1.0f) * 0.5f * io.DisplaySize.x,
                    (1.0f - ndcY) * 0.5f * io.DisplaySize.y,
                    ndcZ
                };
                
                return screen;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                return {-10000, -10000, -1};
            }
        }


        void RecursiveFindRenderers(void* transform, std::vector<void*>& results) {
            if (!transform) return;

            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
            if (!g_RendererClass) g_RendererClass = GetClass("UnityEngine", "Renderer");
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            static Il2CppMethod* getComp = nullptr;
            if (!getComp) getComp = GetMethod((Il2CppClass*)g_ComponentClass, "GetComponent", 1); // Generic? No, use Type
            
            void* rendererType = GetSystemType((Il2CppClass*)g_RendererClass);
            
            static Il2CppMethod* getCompType = nullptr;
            if (!getCompType) getCompType = GetMethod((Il2CppClass*)g_ComponentClass, "GetComponent", 1); // overload with Type argument?
            
            
            void* params[1] = { rendererType };
            void* renderer = RuntimeInvoke(getCompType, transform, params, nullptr);
            if (renderer) {
                results.push_back(renderer);
            }

            static Il2CppMethod* getChildCount = nullptr;
            static Il2CppMethod* getChild = nullptr;
            if (!getChildCount) getChildCount = GetMethod((Il2CppClass*)g_TransformClass, "get_childCount", 0);
            if (!getChild) getChild = GetMethod((Il2CppClass*)g_TransformClass, "GetChild", 1);

            int count = 0;
            void* retCount = RuntimeInvoke(getChildCount, transform, nullptr, nullptr);
            if (retCount) count = *(int*)((char*)retCount + 0x10);

            for (int i = 0; i < count; i++) {
                void* argsChild[1] = { &i };
                void* childM = RuntimeInvoke(getChild, transform, argsChild, nullptr);
                RecursiveFindRenderers(childM, results);
            }
        }

        std::vector<void*> GetRenderers(void* gameObjectOrComponent) {
            std::vector<void*> results;
            if (!gameObjectOrComponent) return results;

            void* transform = nullptr;
            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);
            
            transform = RuntimeInvoke(getTrans, gameObjectOrComponent, nullptr, nullptr);
            if (!transform) return results; // Add verification
            
            RecursiveFindRenderers(transform, results);
            return results;
        }

        void* FindShader(const char* shaderName) {
            if (!g_ShaderClass) g_ShaderClass = GetClass("UnityEngine", "Shader");
            static Il2CppMethod* findMethod = nullptr;
            if (!findMethod) findMethod = GetMethod((Il2CppClass*)g_ShaderClass, "Find", 1);
            
            Il2CppString* str = nullptr; // Need string creation? 
            
            static void* (*il2cpp_string_new)(const char*) = nullptr;
            if (!il2cpp_string_new) {
                il2cpp_string_new = (void* (*)(const char*))GetProcAddress(g_GameAssembly, "il2cpp_string_new");
            }
            if (!il2cpp_string_new) return nullptr;

            void* strObj = il2cpp_string_new(shaderName);
            void* params[1] = { strObj };
            
            return RuntimeInvoke(findMethod, nullptr, params, nullptr);
        }

        void* CreateMaterial(void* shader) {
            if (!shader) return nullptr;
            if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
            
            
            static void* (*il2cpp_object_new)(Il2CppClass*) = nullptr;
             if (!il2cpp_object_new) {
                il2cpp_object_new = (void* (*)(Il2CppClass*))GetProcAddress(g_GameAssembly, "il2cpp_object_new");
            }
            
            void* matObj = il2cpp_object_new((Il2CppClass*)g_MaterialClass);
            
            static Il2CppMethod* ctor = nullptr;
            if (!ctor) ctor = GetMethod((Il2CppClass*)g_MaterialClass, ".ctor", 1); // (Shader)
            
            void* params[1] = { shader };
            RuntimeInvoke(ctor, matObj, params, nullptr);
            
            return matObj;
        }

        void* GetMaterial(void* renderer) {
            if (!renderer) return nullptr;
            if (!g_RendererClass) g_RendererClass = GetClass("UnityEngine", "Renderer");

            static Il2CppMethod* getMat = nullptr;
            if (!getMat) getMat = GetMethod((Il2CppClass*)g_RendererClass, "get_material", 0);

            return RuntimeInvoke(getMat, renderer, nullptr, nullptr);
        }

        void SetMaterial(void* renderer, void* material) {
             if (!renderer) return;
             if (!g_RendererClass) g_RendererClass = GetClass("UnityEngine", "Renderer");
             
             static Il2CppMethod* setMat = nullptr;
             if (!setMat) setMat = GetMethod((Il2CppClass*)g_RendererClass, "set_material", 1);
             
             void* params[1] = { material };
             RuntimeInvoke(setMat, renderer, params, nullptr);
        }

        void SetMaterialColor(void* material, float r, float g, float b, float a) {
            if (!material) return;
             if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
             
             static Il2CppMethod* setError = nullptr;
             static Il2CppMethod* setColor = nullptr;
             if (!setColor) setColor = GetMethod((Il2CppClass*)g_MaterialClass, "set_color", 1);
             
             struct Color { float r,g,b,a; };
             Color c = {r,g,b,a};
             void* params[1] = { &c };
             
             RuntimeInvoke(setColor, material, params, nullptr);
        }

        void SetMaterialInt(void* material, const char* propName, int value) {
            if (!material) return;
            if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
            
            static Il2CppMethod* setInt = nullptr;
            if (!setInt) setInt = GetMethod((Il2CppClass*)g_MaterialClass, "SetInt", 2);
            if (!setInt) return;
            
            static void* (*il2cpp_string_new)(const char*) = nullptr;
            if (!il2cpp_string_new) {
                il2cpp_string_new = (void* (*)(const char*))GetProcAddress(g_GameAssembly, "il2cpp_string_new");
            }
            if (!il2cpp_string_new) return;
            
            void* strName = il2cpp_string_new(propName);
            void* params[2] = { strName, &value };
            RuntimeInvoke(setInt, material, params, nullptr);
        }

        void SetMaterialFloat(void* material, const char* propName, float value) {
            if (!material) return;
            if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
            
            static Il2CppMethod* setFloat = nullptr;
            if (!setFloat) setFloat = GetMethod((Il2CppClass*)g_MaterialClass, "SetFloat", 2);
            if (!setFloat) return;
            
            static void* (*il2cpp_string_new)(const char*) = nullptr;
            if (!il2cpp_string_new) {
                il2cpp_string_new = (void* (*)(const char*))GetProcAddress(g_GameAssembly, "il2cpp_string_new");
            }
            if (!il2cpp_string_new) return;
            
            void* strName = il2cpp_string_new(propName);
            void* params[2] = { strName, &value };
            RuntimeInvoke(setFloat, material, params, nullptr);
        }

        void SetMaterialZTestAlways(void* material) {
            if (!material) return;
            SetMaterialInt(material, "_ZTest", 8);
            SetMaterialInt(material, "_ZWrite", 0);
        }

        void SetMaterialGlow(void* material, float r, float g, float b, float intensity) {
            if (!material) return;
            struct Color { float r,g,b,a; };
            Color emissionColor = { r * intensity, g * intensity, b * intensity, 1.0f };
            
            if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
            
            static Il2CppMethod* setColor = nullptr;
            if (!setColor) setColor = GetMethod((Il2CppClass*)g_MaterialClass, "SetColor", 2);
            
            if (setColor) {
                static void* (*il2cpp_string_new)(const char*) = nullptr;
                if (!il2cpp_string_new) {
                    il2cpp_string_new = (void* (*)(const char*))GetProcAddress(g_GameAssembly, "il2cpp_string_new");
                }
                if (il2cpp_string_new) {
                    void* strName = il2cpp_string_new("_EmissionColor");
                    void* params[2] = { strName, &emissionColor };
                    RuntimeInvoke(setColor, material, params, nullptr);
                }
            }
            
            static Il2CppMethod* enableKeyword = nullptr;
            if (!enableKeyword) enableKeyword = GetMethod((Il2CppClass*)g_MaterialClass, "EnableKeyword", 1);
            if (enableKeyword) {
                static void* (*il2cpp_string_new)(const char*) = nullptr;
                if (!il2cpp_string_new) {
                    il2cpp_string_new = (void* (*)(const char*))GetProcAddress(g_GameAssembly, "il2cpp_string_new");
                }
                if (il2cpp_string_new) {
                    void* kwStr = il2cpp_string_new("_EMISSION");
                    void* params[1] = { kwStr };
                    RuntimeInvoke(enableKeyword, material, params, nullptr);
                }
            }
        }

        void SetMaterialEmissionColor(void* material, float r, float g, float b, float a) {
            if (!material) return;
            SetMaterialGlow(material, r, g, b, 1.0f);
        }

        void SetMaterialGlowStrength(void* material, float strength) {
            if (!material) return;
            if (!g_MaterialClass) g_MaterialClass = GetClass("UnityEngine", "Material");
            
            static Il2CppMethod* setFloat = nullptr;
            if (!setFloat) setFloat = GetMethod((Il2CppClass*)g_MaterialClass, "SetFloat", 2);
            
            static void* (*il2cpp_string_new)(const char*) = nullptr;
            if (!il2cpp_string_new) {
                il2cpp_string_new = (void* (*)(const char*))GetProcAddress(g_GameAssembly, "il2cpp_string_new");
            }
            
            void* propName = il2cpp_string_new("_GlowStrength");
            void* params[2] = { propName, &strength };
            
            RuntimeInvoke(setFloat, material, params, nullptr);
        }

        BodyPart GetRendererBodyPart(void* renderer) {
            if (!renderer) return BodyPart_None;
            
            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);
            
            void* transform = RuntimeInvoke(getTrans, renderer, nullptr, nullptr);
            if (!transform) return BodyPart_None;
            
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");
            static Il2CppMethod* getName = nullptr;
            if (!getName) getName = GetMethod((Il2CppClass*)g_TransformClass, "get_name", 0);
            
            void* nameObj = RuntimeInvoke(getName, transform, nullptr, nullptr);
            if (!nameObj) return BodyPart_None;
            
            const char* name = *(const char**)((char*)nameObj + 0x10);
            if (!name) return BodyPart_None;
            
            std::string nameStr(name);
            for (char& c : nameStr) c = tolower(c);
            
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
            
            std::vector<void*> allRenderers = GetRenderers(gameObject);
            
            for (void* renderer : allRenderers) {
                if (GetRendererBodyPart(renderer) == part) {
                    results.push_back(renderer);
                }
            }
            
            return results;
        }
        
        
        void* GetMitaNavMeshAgent() {
            void* animator = GetMitaAnimator();
            if (!animator) return nullptr;
            
            static void* (*get_transform)(void*) = nullptr;
            if (!get_transform && sdk::il2cpp_resolve_icall) 
                get_transform = (void* (*)(void*))sdk::il2cpp_resolve_icall("UnityEngine.Component::get_transform");
            
            static void* (*get_parent)(void*) = nullptr;
            if (!get_parent && sdk::il2cpp_resolve_icall) 
                get_parent = (void* (*)(void*))sdk::il2cpp_resolve_icall("UnityEngine.Transform::get_parent");
                
            static void* (*get_gameObject)(void*) = nullptr;
            if (!get_gameObject && sdk::il2cpp_resolve_icall) 
                get_gameObject = (void* (*)(void*))sdk::il2cpp_resolve_icall("UnityEngine.Component::get_gameObject");

            if (!get_transform || !get_gameObject) return nullptr;
            
            void* transform = get_transform(animator);
            if (transform && get_parent) {
                void* parent = get_parent(transform);
                if (parent) transform = parent; // Check parent (Root)
            }
            
            void* gameObject = get_gameObject(transform);
            if (!gameObject) return nullptr;
            
            static void* (*get_component)(void*, void*) = nullptr;
            if (!get_component && sdk::il2cpp_resolve_icall) 
                get_component = (void* (*)(void*, void*))sdk::il2cpp_resolve_icall("UnityEngine.GameObject::GetComponent");
            
            static void* navMeshAgentType = nullptr;
            if (!navMeshAgentType && sdk::il2cpp_class_get_type && sdk::il2cpp_type_get_object) {
                void* agentClass = sdk::GetClass("UnityEngine.AI", "NavMeshAgent");
                if (agentClass) {
                    Il2CppType* type = sdk::il2cpp_class_get_type((Il2CppClass*)agentClass);
                    if (type) navMeshAgentType = sdk::il2cpp_type_get_object(type);
                }
            }
            
            if (!get_component || !navMeshAgentType) return nullptr;
            
            return get_component(gameObject, navMeshAgentType);
        }

        void SetAnimatorApplyRootMotion(void* animator, bool apply) {
            if (!animator || !sdk::il2cpp_resolve_icall) return;
            static void (*set_applyRootMotion)(void*, bool) = nullptr;
            if (!set_applyRootMotion) set_applyRootMotion = (void (*)(void*, bool))sdk::il2cpp_resolve_icall("UnityEngine.Animator::set_applyRootMotion");
            if (set_applyRootMotion) set_applyRootMotion(animator, apply);
        }

        void SetAgentSpeed(void* agent, float speed) {
            if (!agent || !sdk::il2cpp_resolve_icall) return;
            static void (*set_speed)(void*, float) = nullptr;
            if (!set_speed) set_speed = (void (*)(void*, float))sdk::il2cpp_resolve_icall("UnityEngine.AI.NavMeshAgent::set_speed");
            if (set_speed) set_speed(agent, speed);
        }

        void SetAgentAcceleration(void* agent, float acceleration) {
            if (!agent || !sdk::il2cpp_resolve_icall) return;
            static void (*set_acceleration)(void*, float) = nullptr;
            if (!set_acceleration) set_acceleration = (void (*)(void*, float))sdk::il2cpp_resolve_icall("UnityEngine.AI.NavMeshAgent::set_acceleration");
            if (set_acceleration) set_acceleration(agent, acceleration);
        }

        float GetAgentSpeed(void* agent) {
            if (!agent || !sdk::il2cpp_resolve_icall) return 0.0f;
            static float (*get_speed)(void*) = nullptr;
            if (!get_speed) get_speed = (float (*)(void*))sdk::il2cpp_resolve_icall("UnityEngine.AI.NavMeshAgent::get_speed");
            if (get_speed) return get_speed(agent);
            return 0.0f;
        }
        
        
        static void* g_CachedMitaCycle = nullptr;
        static int g_MitaCycleRefreshTimer = 0;
        
        void* GetMitaCycle() {
            if (g_MitaCycleRefreshTimer <= 0 || !g_CachedMitaCycle) {
                g_CachedMitaCycle = FindObjectOfType("MitaCycle");
                g_MitaCycleRefreshTimer = 120; // ~2s at 60fps
            } else {
                g_MitaCycleRefreshTimer--;
            }
            return g_CachedMitaCycle;
        }
        
        int GetOutfitCount() {
            void* mitaCycle = GetMitaCycle();
            if (!mitaCycle || !IsValidPtr(mitaCycle)) return 0;
            
            void* themeOptions = *(void**)((uintptr_t)mitaCycle + 0x30);
            if (!themeOptions || !IsValidPtr(themeOptions)) return 0;
            
            int count = *(int*)((uintptr_t)themeOptions + 0x18);
            return count;
        }
        
        int GetCurrentOutfit() {
            void* mitaCycle = GetMitaCycle();
            if (!mitaCycle || !IsValidPtr(mitaCycle)) return -1;
            
            int index = *(int*)((uintptr_t)mitaCycle + 0x80);
            return index;
        }
        
        void SetGameObjectActive(void* gameObject, bool active) {
            if (!gameObject || !IsValidPtr(gameObject)) return;
            
            static Il2CppClass* goClass = nullptr;
            if (!goClass) goClass = GetClass("UnityEngine", "GameObject");
            if (!goClass) return;
            
            static Il2CppMethod* setActive = nullptr;
            if (!setActive) setActive = GetMethod(goClass, "SetActive", 1);
            if (!setActive) return;
            
            void* params[1] = { &active };
            RuntimeInvoke(setActive, gameObject, params, nullptr);
        }
        
        bool SetOutfit(int index) {
            void* mitaCycle = GetMitaCycle();
            if (!mitaCycle || !IsValidPtr(mitaCycle)) return false;
            
            int count = GetOutfitCount();
            if (index < 0 || index >= count) return false;
            
            int currentIndex = GetCurrentOutfit();
            if (index == currentIndex) return true; // Already at this outfit
            
            void* themeOptions = *(void**)((uintptr_t)mitaCycle + 0x30);
            if (!themeOptions || !IsValidPtr(themeOptions)) return false;
            
            void** items = *(void***)((uintptr_t)themeOptions + 0x10);
            if (!items) return false;
            
            if (currentIndex >= 0 && currentIndex < count) {
                void* currentCycler = items[currentIndex];
                if (currentCycler && IsValidPtr(currentCycler)) {
                    void* currentMita = *(void**)((uintptr_t)currentCycler + 0x20);
                    if (currentMita && IsValidPtr(currentMita)) {
                        SetGameObjectActive(currentMita, false);
                    }
                }
            }
            
            void* newCycler = items[index];
            if (newCycler && IsValidPtr(newCycler)) {
                void* newMita = *(void**)((uintptr_t)newCycler + 0x20);
                if (newMita && IsValidPtr(newMita)) {
                    SetGameObjectActive(newMita, true);
                }
            }
            
            *(int*)((uintptr_t)mitaCycle + 0x80) = index;
            
            return true;
        }
        
        void CycleNextOutfit() {
            int count = GetOutfitCount();
            if (count <= 0) return;
            
            int current = GetCurrentOutfit();
            int next = (current + 1) % count;
            SetOutfit(next);
        }
        
        void CyclePreviousOutfit() {
            int count = GetOutfitCount();
            if (count <= 0) return;
            
            int current = GetCurrentOutfit();
            int prev = (current - 1 + count) % count;
            SetOutfit(prev);
        }
        
        std::vector<std::string> GetOutfitNames() {
            std::vector<std::string> names;
            
            void* mitaCycle = GetMitaCycle();
            if (!mitaCycle || !IsValidPtr(mitaCycle)) return names;
            
            int count = GetOutfitCount();
            if (count <= 0) return names;
            
            void* themeOptions = *(void**)((uintptr_t)mitaCycle + 0x30);
            if (!themeOptions || !IsValidPtr(themeOptions)) return names;
            
            void** items = *(void***)((uintptr_t)themeOptions + 0x10);
            if (!items) return names;
            
            for (int i = 0; i < count; i++) {
                void* cycler = items[i];
                if (!cycler || !IsValidPtr(cycler)) {
                    names.push_back("Unknown");
                    continue;
                }
                
                names.push_back("Outfit " + std::to_string(i + 1));
            }
            
            return names;
        }


        void* GetPlayerCameraObject() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) {
                return GetMainCamera(); // Fallback
            }

            __try {
                void* cam = *(void**)((uintptr_t)pm + 0x28);
                if (cam && IsValidPtr(cam)) {
                    return cam;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }

            return GetMainCamera();
        }

        float GetCameraFOV(void* camera) {
            if (!camera || !IsValidPtr(camera)) return 0.0f;

            if (!il2cpp_resolve_icall) {
                il2cpp_resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
            }
            if (!il2cpp_resolve_icall) return 0.0f;

            static float (*get_fov)(void*) = nullptr;
            if (!get_fov) {
                get_fov = (float (*)(void*))il2cpp_resolve_icall("UnityEngine.Camera::get_fieldOfView");
            }
            if (!get_fov) return 0.0f;

            __try {
                return get_fov(camera);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return 0.0f;
            }
        }

        void SetCameraFOV(void* camera, float fov) {
            if (!camera || !IsValidPtr(camera)) return;

            if (!il2cpp_resolve_icall) {
                il2cpp_resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
            }
            if (!il2cpp_resolve_icall) return;

            static void (*set_fov)(void*, float) = nullptr;
            if (!set_fov) {
                set_fov = (void (*)(void*, float))il2cpp_resolve_icall("UnityEngine.Camera::set_fieldOfView");
            }
            if (!set_fov) return;

            __try {
                set_fov(camera, fov);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        void* GetKiriMove() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) return nullptr;

            __try {
                void* move = *(void**)((uintptr_t)pm + 0x58);
                if (move && IsValidPtr(move)) {
                    return move;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return nullptr;
            }

            return nullptr;
        }

        float GetJumpHeight(void* kiriMove) {
            return 0.0f;
        }

        void SetJumpHeight(void* kiriMove, float height) {
            return;
        }

        void* GetPlayerLook() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) return nullptr;
            
            __try {
                void* look = *(void**)((uintptr_t)pm + 0x60);
                if (look && IsValidPtr(look)) return look;
            } __except(EXCEPTION_EXECUTE_HANDLER) {}
            return nullptr;
        }

        void SetBehaviourEnabled(void* behaviour, bool enabled) {
            if (!behaviour) return;
            typedef void(__stdcall* fnSetEnabled)(void*, bool);
            static fnSetEnabled s_fn = nullptr;
            
            if (!s_fn) {
                if (!resolve_icall) resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
                if (resolve_icall) s_fn = (fnSetEnabled)resolve_icall("UnityEngine.Behaviour::set_enabled(System.Boolean)");
            }
            if (s_fn) s_fn(behaviour, enabled);
        }

        void* GetPlayerCameraScript() {
             return FindObjectOfType("PlayerCamera");
        }


        std::vector<void*> GetActiveTweens() {
            std::vector<void*> tweens;
            
            static Il2CppClass* tmClass = nullptr;
            if (!tmClass) tmClass = GetClass("DG.Tweening.Core", "TweenManager");
            
            if (!tmClass) {
                static Il2CppClass* dotweenClass = nullptr;
                if (!dotweenClass) dotweenClass = GetClass("DG.Tweening", "DOTween");
            }

            if (!tmClass) return tweens;

            static Il2CppField* activeTweensField = nullptr;
            if (!activeTweensField) activeTweensField = GetField(tmClass, "_activeTweens");
            
            
            if (!activeTweensField) return tweens;

            void* list = nullptr;
            il2cpp_field_static_get_value(activeTweensField, &list);
            
            if (!list) return tweens;

            
            int size = 0;
            __try {
                size = *(int*)((uintptr_t)list + 0x18);
                void* items = *(void**)((uintptr_t)list + 0x10);
                
                if (items && size > 0) {
                    void** ptrs = (void**)((uintptr_t)items + 0x20);
                    
                    for (int i = 0; i < size; i++) {
                        if (ptrs[i]) {
                            tweens.push_back(ptrs[i]);
                        }
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
            
            return tweens;
        }

        std::vector<Vector3> GetTweenPathPoints(void* tween) {
            std::vector<Vector3> points;
            if (!tween) return points;

            
            static Il2CppClass* pathClass = nullptr;
            if (!pathClass) pathClass = GetClass("DG.Tweening.Plugins.Core.PathCore", "Path");
            
            if (!pathClass) return points;

            static int pathFieldOffset = -1;
            
            if (pathFieldOffset == -1) {
                Il2CppClass* klass = *(Il2CppClass**)tween;
                
                
            }

            
            static Il2CppClass* tweenerCoreClass = nullptr; // We can't easily get generic class by name
            
            
            
            Il2CppClass* klass = *(Il2CppClass**)tween;
            
            void* pathObj = nullptr;
            
            Il2CppField* endValField = GetField(klass, "endValue");
            if (endValField) {
                 il2cpp_field_get_value(tween, endValField, &pathObj);
            }
            
            if (!pathObj) {
                 Il2CppField* startValField = GetField(klass, "startValue");
                 if (startValField) {
                     il2cpp_field_get_value(tween, startValField, &pathObj);
                 }
            }
            
            if (!pathObj) return points;
            
            Il2CppClass* objClass = *(Il2CppClass**)pathObj;
            if (!objClass) return points;
            
            
            Il2CppField* wpsField = GetField(objClass, "wps"); // Waypoints
            if (!wpsField) return points;
            
            void* wpsArray = nullptr;
            il2cpp_field_get_value(pathObj, wpsField, &wpsArray);
            
            if (wpsArray) {
                int count = *(int*)((uintptr_t)wpsArray + 0x18);
                if (count > 0 && count < 1000) { // Sanity check
                    Vector3* src = (Vector3*)((uintptr_t)wpsArray + 0x20);
                    for (int i = 0; i < count; i++) {
                        points.push_back(src[i]);
                    }
                }
            }
            
            return points;
        }



        Vector3 GetRigidbodyVelocity(void* rb) {
            if (!rb) return {0,0,0};
            static Il2CppClass* rbClass = nullptr;
            if (!rbClass) rbClass = GetClass("UnityEngine", "Rigidbody");
            
            static Il2CppMethod* getVel = nullptr;
            if (!getVel) getVel = GetMethod(rbClass, "get_velocity", 0);
            
            Vector3 res = {0,0,0};
            if (getVel) {
                void* ret = RuntimeInvoke(getVel, rb, nullptr, nullptr);
                if (ret) {
                    res = *(Vector3*)((char*)ret + 0x10);
                }
            }
            return res;
        }

        std::vector<void*> FindObjectsOfTypeAll(const char* className) {
             std::vector<void*> results;
             Il2CppClass* targetClass = GetClass("", className);
             if (!targetClass) targetClass = GetClass("UnityEngine", className); // Try UnityEngine namespace
             if (!targetClass) return results;

             void* systemType = GetSystemType(targetClass);
             if (!systemType) return results;
             
             static Il2CppClass* objectClass = nullptr;
             if (!objectClass) objectClass = GetClass("UnityEngine", "Object");
             
             static Il2CppMethod* findAll = nullptr;
             if (!findAll) findAll = GetMethod(objectClass, "FindObjectsOfType", 1); // (Type) -> Object[]
             
             if (!findAll) return results;
             
             void* params[1] = { systemType };
             void* arrayObj = RuntimeInvoke(findAll, nullptr, params, nullptr);
             
             if (!arrayObj) return results;
             
             
             uint32_t length = *(uint32_t*)((char*)arrayObj + 0x18);
             void** items = (void**)((char*)arrayObj + 0x20);
             
             if (length > 10000) length = 10000;

             for (uint32_t i = 0; i < length; i++) {
                 if (items[i]) results.push_back(items[i]);
             }
             
             return results;
        }

        const char* GetObjectName(void* object) {
            if (!object) return "null";
            static Il2CppClass* objectClass = nullptr;
            if (!objectClass) objectClass = GetClass("UnityEngine", "Object");
            
            static Il2CppMethod* getName = nullptr;
            if (!getName) getName = GetMethod(objectClass, "get_name", 0);
            
            void* strObj = RuntimeInvoke(getName, object, nullptr, nullptr);
            if (!strObj) return "null";
            
            int length = *(int*)((char*)strObj + 0x10);
            wchar_t* chars = (wchar_t*)((char*)strObj + 0x14);
            
            static char buffer[256];
            WideCharToMultiByte(CP_UTF8, 0, chars, length, buffer, sizeof(buffer)-1, NULL, NULL);
            buffer[length < 255 ? length : 255] = 0;
            
            return buffer;
        }
        
        const char* GetLayerName(int layer) {
             static Il2CppClass* layerMaskClass = nullptr;
             if (!layerMaskClass) layerMaskClass = GetClass("UnityEngine", "LayerMask");
             
             static Il2CppMethod* layerToName = nullptr;
             if (!layerToName) layerToName = GetMethod(layerMaskClass, "LayerToName", 1);
             
             void* args[1] = { &layer };
             void* strObj = RuntimeInvoke(layerToName, nullptr, args, nullptr);
             if (!strObj) return "Unknown";
             
             int length = *(int*)((char*)strObj + 0x10);
             wchar_t* chars = (wchar_t*)((char*)strObj + 0x14);
             
             static char buffer[64];
             WideCharToMultiByte(CP_UTF8, 0, chars, length, buffer, sizeof(buffer)-1, NULL, NULL);
             buffer[length < 63 ? length : 63] = 0;
             return buffer;
        }


        Vector3 GetCameraPosition(void* camera) {
            if (!camera) return {0,0,0};
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);

            void* transform = RuntimeInvoke(getTrans, camera, nullptr, nullptr);
            if (!transform) return {0,0,0};

            return GetTransformPosition(transform);
        }

        void SetCameraPosition(void* camera, Vector3 pos) {
            if (!camera) return;
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);

            void* transform = RuntimeInvoke(getTrans, camera, nullptr, nullptr);
            if (!transform) return;

            static Il2CppMethod* setPos = nullptr;
            if (!setPos) setPos = GetMethod((Il2CppClass*)g_TransformClass, "set_position", 1);

            if (setPos) {
                void* params[1] = { &pos };
                RuntimeInvoke(setPos, transform, params, nullptr);
            }
        }

        Quaternion GetCameraRotation(void* camera) {
            if (!camera) return {0,0,0,1};
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);

            void* transform = RuntimeInvoke(getTrans, camera, nullptr, nullptr);
            if (!transform) return {0,0,0,1};

            static Il2CppMethod* getRot = nullptr;
            if (!getRot) getRot = GetMethod((Il2CppClass*)g_TransformClass, "get_rotation", 0);

            Quaternion res = {0,0,0,1};
            if (getRot) {
                void* ret = RuntimeInvoke(getRot, transform, nullptr, nullptr);
                if (ret) {
                    res = *(Quaternion*)((char*)ret + 0x10);
                }
            }
            return res;
        }

        Vector3 GetCameraRotationEuler(void* camera) {
            if (!camera) return {0,0,0};
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);

            void* transform = RuntimeInvoke(getTrans, camera, nullptr, nullptr);
            if (!transform) return {0,0,0};

            static Il2CppMethod* getEuler = nullptr;
            if (!getEuler) getEuler = GetMethod((Il2CppClass*)g_TransformClass, "get_eulerAngles", 0);

            Vector3 res = {0,0,0};
            if (getEuler) {
                void* ret = RuntimeInvoke(getEuler, transform, nullptr, nullptr);
                if (ret) {
                    res = *(Vector3*)((char*)ret + 0x10);
                }
            }
            return res;
        }

        void SetCameraRotation(void* camera, Quaternion rot) {
            if (!camera) return;
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);

            void* transform = RuntimeInvoke(getTrans, camera, nullptr, nullptr);
            if (!transform) return;

            static Il2CppMethod* setRot = nullptr;
            if (!setRot) setRot = GetMethod((Il2CppClass*)g_TransformClass, "set_rotation", 1);

            if (setRot) {
                void* params[1] = { &rot };
                RuntimeInvoke(setRot, transform, params, nullptr);
            }
        }

        Vector3 GetCameraForward(void* camera) {
            if (!camera) return {0,0,1};
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);

            void* transform = RuntimeInvoke(getTrans, camera, nullptr, nullptr);
            if (!transform) return {0,0,1};

            static Il2CppMethod* getFwd = nullptr;
            if (!getFwd) getFwd = GetMethod((Il2CppClass*)g_TransformClass, "get_forward", 0);

            Vector3 res = {0,0,1};
            if (getFwd) {
                void* ret = RuntimeInvoke(getFwd, transform, nullptr, nullptr);
                if (ret) {
                    res = *(Vector3*)((char*)ret + 0x10);
                }
            }
            return res;
        }


        typedef void(__stdcall* fnGetPositionInjected)(void* transform, Vector3* ret);
        typedef void(__stdcall* fnSetPositionInjected)(void* transform, const Vector3* value);
        typedef void(__stdcall* fnGetRotationInjected)(void* transform, Quaternion* ret);
        typedef void(__stdcall* fnSetRotationInjected)(void* transform, const Quaternion* value);
        
        static fnGetPositionInjected s_GetPositionInjected = nullptr;
        static fnSetPositionInjected s_SetPositionInjected = nullptr;
        static fnGetRotationInjected s_GetRotationInjected = nullptr;
        static fnSetRotationInjected s_SetRotationInjected = nullptr;

        void* GetCameraTransform(void* camera) {
            if (!camera) return nullptr;
            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");

            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);

            return RuntimeInvoke(getTrans, camera, nullptr, nullptr);
        }

        Vector3 GetTransformPositionFast(void* transform) {
            if (!transform) return {0,0,0};
            
            if (!s_GetPositionInjected) {
                if (!resolve_icall) resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
                if (resolve_icall) {
                    s_GetPositionInjected = (fnGetPositionInjected)resolve_icall("UnityEngine.Transform::get_position_Injected(UnityEngine.Vector3&)");
                }
            }
            
            Vector3 result = {0,0,0};
            if (s_GetPositionInjected) {
                __try {
                    s_GetPositionInjected(transform, &result);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return {0,0,0};
                }
            } else {
                return GetTransformPosition(transform);
            }
            return result;
        }

        void SetTransformPositionFast(void* transform, Vector3 pos) {
            if (!transform) return;
            
            if (!s_SetPositionInjected) {
                if (!resolve_icall) resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
                if (resolve_icall) {
                    s_SetPositionInjected = (fnSetPositionInjected)resolve_icall("UnityEngine.Transform::set_position_Injected(UnityEngine.Vector3&)");
                }
            }
            
            if (s_SetPositionInjected) {
                __try {
                    s_SetPositionInjected(transform, &pos);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                }
            }
        }

        Quaternion GetTransformRotationFast(void* transform) {
            if (!transform) return {0,0,0,1};
            
            if (!s_GetRotationInjected) {
                if (!resolve_icall) resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
                if (resolve_icall) {
                    s_GetRotationInjected = (fnGetRotationInjected)resolve_icall("UnityEngine.Transform::get_rotation_Injected(UnityEngine.Quaternion&)");
                }
            }
            
            Quaternion result = {0,0,0,1};
            if (s_GetRotationInjected) {
                __try {
                    s_GetRotationInjected(transform, &result);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return {0,0,0,1};
                }
            }
            return result;
        }

        void SetTransformRotationFast(void* transform, Quaternion rot) {
            if (!transform) return;
            
            if (!s_SetRotationInjected) {
                if (!resolve_icall) resolve_icall = (t_il2cpp_resolve_icall)GetProcAddress(GetModuleHandleA("GameAssembly.dll"), "il2cpp_resolve_icall");
                if (resolve_icall) {
                    s_SetRotationInjected = (fnSetRotationInjected)resolve_icall("UnityEngine.Transform::set_rotation_Injected(UnityEngine.Quaternion&)");
                }
            }
            
            if (s_SetRotationInjected) {
                __try {
                    s_SetRotationInjected(transform, &rot);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                }
            }
        }


        void* GetPlayerFeetTransform() {
            void* move = GetPlayerMovement();
            if (!move || !IsValidPtr(move)) return nullptr;

            __try {
                void* feet = *(void**)((uintptr_t)move + 0x40);
                if (!IsValidPtr(feet)) return nullptr;
                return feet;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return nullptr;
            }
        }

        void SetPlayerPositionDirect(Vector3 pos) {
            void* feet = GetPlayerFeetTransform();
            if (!feet) return;

            SetTransformPositionFast(feet, pos);
        }

    } // namespace game
} // namespace sdk
