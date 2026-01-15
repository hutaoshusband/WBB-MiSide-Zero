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

    HMODULE g_GameAssembly = nullptr;
    bool g_Initialized = false;

    // Basic pointer validation
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
    
    // Thread-local flag to track if this thread is attached to IL2CPP runtime
    static thread_local bool t_ThreadAttached = false;
    static thread_local Il2CppThread* t_AttachedThread = nullptr;

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
        LOAD_PROC(il2cpp_field_set_value);
        LOAD_PROC(il2cpp_runtime_invoke);
        LOAD_PROC(il2cpp_image_get_name);
        LOAD_PROC(il2cpp_class_get_type);
        LOAD_PROC(il2cpp_type_get_object);

        // Attach the initializing thread
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
        // Already attached on this thread
        if (t_ThreadAttached && t_AttachedThread) {
            return true;
        }
        
        // SDK not initialized
        if (!g_Initialized || !il2cpp_thread_attach || !il2cpp_domain_get) {
            return false;
        }
        
        // Attach this thread to the IL2CPP runtime
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

    // ===========================================
    // GAME IMPL
    // ===========================================

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
            if (!pm || !IsValidPtr(pm)) return nullptr;
            
            if (!g_PlayerManagerClass) g_PlayerManagerClass = GetClass("", "PlayerManager");
            if (!g_PlayerManagerClass) return nullptr;

            static Il2CppField* moveField = GetField((Il2CppClass*)g_PlayerManagerClass, "move");
            if (!moveField) return nullptr;
            
            void* move = nullptr;
            GetFieldValueInternal(pm, moveField, &move);
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

             // RVA: 0x19359E0 for UnityEngine.Object.FindObjectOfType(Type)
             static uintptr_t rva = 0x19359E0;
             static void* (*FindObjectOfTypeFunc)(void*) = nullptr;
             
             if (!FindObjectOfTypeFunc) {
                 FindObjectOfTypeFunc = (void* (*)(void*))((uintptr_t)g_GameAssembly + rva);
             }

             return FindObjectOfTypeFunc(systemType);
        }

        void* GetMitaManager() {
             // Don't cache permanently - Mita can be destroyed/recreated
             // Use a simple timer instead
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
                // kiriMoveBasic: walkSpeed at 0x1C.
                return *(float*)((uintptr_t)movement + 0x1C);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return 0.0f;
            }
        }

        void SetSpeed(void* movement, float speed) {
            if (!IsValidPtr(movement)) return;
            __try {
                // kiriMoveBasic: walkSpeed at 0x1C.
                *(float*)((uintptr_t)movement + 0x1C) = speed;
                // Also ensure canMove is on (0x18)
                *(bool*)((uintptr_t)movement + 0x18) = true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                // Log or ignore
            }
        }

        void* GetPlayerRigidbody() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) return nullptr;
            
            if (!g_PlayerManagerClass) g_PlayerManagerClass = GetClass("", "PlayerManager");
            if (!g_PlayerManagerClass) return nullptr;

            static Il2CppField* rbField = GetField((Il2CppClass*)g_PlayerManagerClass, "rb");
            if (!rbField) return nullptr;

            void* rb = nullptr;
            GetFieldValueInternal(pm, rbField, &rb);
            return rb;
        }

        void* GetPlayerCollider() {
            void* pm = GetPlayerManager();
            if (!pm || !IsValidPtr(pm)) return nullptr;
            
            if (!g_PlayerManagerClass) g_PlayerManagerClass = GetClass("", "PlayerManager");
            if (!g_PlayerManagerClass) return nullptr;

            static Il2CppField* colField = GetField((Il2CppClass*)g_PlayerManagerClass, "col");
            if (!colField) return nullptr;

            void* col = nullptr;
            GetFieldValueInternal(pm, colField, &col);
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

        // ===========================================
        // MANUAL W2S IMPLEMENTATION
        // ===========================================
        
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
            void* cam = GetMainCamera();
            if (!cam) cam = GetPlayerCamera();
            if (!cam) return {-10000, -10000, -1};

            Matrix4x4 view = GetViewMatrix(cam);
            Matrix4x4 proj = GetProjectionMatrix(cam);
            Matrix4x4 vp = Matrix4x4::Multiply(proj, view); // VP = P * V

            // Transform [x,y,z,1]
            float x = worldPos.x * vp(0, 0) + worldPos.y * vp(0, 1) + worldPos.z * vp(0, 2) + vp(0, 3);
            float y = worldPos.x * vp(1, 0) + worldPos.y * vp(1, 1) + worldPos.z * vp(1, 2) + vp(1, 3);
            float z = worldPos.x * vp(2, 0) + worldPos.y * vp(2, 1) + worldPos.z * vp(2, 2) + vp(2, 3);
            float w = worldPos.x * vp(3, 0) + worldPos.y * vp(3, 1) + worldPos.z * vp(3, 2) + vp(3, 3);

            if (w < 0.1f) return {-10000, -10000, -1}; // Behind camera

            Vector3 screen = {x/w, y/w, z/w};
            
            // Convert NDC to Screen
            ImGuiIO& io = ImGui::GetIO();
            screen.x = (screen.x + 1.0f) * 0.5f * io.DisplaySize.x;
            screen.y = (1.0f - screen.y) * 0.5f * io.DisplaySize.y; // Flip Y for ImGui
            
            return screen;
        }

        // ===========================================
        // CHAMS HELPERS
        // ===========================================

        void RecursiveFindRenderers(void* transform, std::vector<void*>& results) {
            if (!transform) return;

            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
            if (!g_RendererClass) g_RendererClass = GetClass("UnityEngine", "Renderer");
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");

            // Check if this transform has a renderer
            static Il2CppMethod* getComp = nullptr;
            if (!getComp) getComp = GetMethod((Il2CppClass*)g_ComponentClass, "GetComponent", 1); // Generic? No, use Type
            // Actually GetComponent(Type) RVA is 0x192E210?
            // Let's use RuntimeInvoke on 'GetComponent(Type)'
            
            // We need Type object.
            void* rendererType = GetSystemType((Il2CppClass*)g_RendererClass);
            
            // GetComponent(Type)
            static Il2CppMethod* getCompType = nullptr;
            if (!getCompType) getCompType = GetMethod((Il2CppClass*)g_ComponentClass, "GetComponent", 1); // overload with Type argument?
            
            // To call GetComponent(Type), we need to find it by name and args count
            // Note: GetComponent has many overloads. 
            // 0 args = Generic (can't use easily)
            // 1 arg = Type typeof(T) usually.
            
            void* params[1] = { rendererType };
            void* renderer = RuntimeInvoke(getCompType, transform, params, nullptr);
            if (renderer) {
                results.push_back(renderer);
            }

            // Iterate children
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

            // Get Transform first
            void* transform = nullptr;
            // Check if it's already a transform? Hard to tell. Assume Component.
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
            // In il2cpp, strings are objects. We need il2cpp_string_new
            // But we don't have it imported in sdk.cpp explicitly?
            // It should be exported by GameAssembly. 
            // Actually, we can just pass a C-string if we are lucky? NO.
            // We need to resolve il2cpp_string_new.
            
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
            
            // We need to call Constructor. RuntimeInvoke on .ctor?
            // But we need to allocate the object first? 
            // In Unity/Il2Cpp, creating a new object is usually done via il2cpp_object_new then constructor.
            // But Material(Shader) might be special.
            
            // Better way: use object_new
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
             // Check if "SetColor" exists or property "color"?
             // Material has "set_color"
             static Il2CppMethod* setColor = nullptr;
             if (!setColor) setColor = GetMethod((Il2CppClass*)g_MaterialClass, "set_color", 1);
             
             // Color struct? We need to pass it.
             struct Color { float r,g,b,a; };
             Color c = {r,g,b,a};
             void* params[1] = { &c };
             
             RuntimeInvoke(setColor, material, params, nullptr);
        }
    }
}
