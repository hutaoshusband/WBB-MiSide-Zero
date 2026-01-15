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

        g_GameAssembly = GetModuleHandleA("GameAssembly.dll");
        if (!g_GameAssembly) return false;

        #define LOAD_PROC(name) name = (t_##name)GetProcAddress(g_GameAssembly, #name); if(!name) return false;

        LOAD_PROC(il2cpp_domain_get);
        LOAD_PROC(il2cpp_thread_attach);
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
             // Test PlayerManager first to verify system
             FindObjectOfType("PlayerManager"); // This updates the log
             
             // Then try MitaManager
             void* mita = FindObjectOfType("MitaManager");
             return mita;
        }

        Vector3 GetTransformPosition(void* transform) {
            if (!transform) return {0,0,0};
            if (!g_TransformClass) g_TransformClass = GetClass("UnityEngine", "Transform");
            
            static Il2CppMethod* getPos = nullptr;
            if (!getPos) getPos = GetMethod((Il2CppClass*)g_TransformClass, "get_position", 0);
            if (!getPos) return {0,0,0};

            Vector3 res;
            void* ret = RuntimeInvoke(getPos, transform, nullptr, nullptr);
            // IL2CPP struct return: usually pointer to result or unboxed
            // Vector3 is a value type. il2cpp_runtime_invoke returns a boxed object for value types.
            // We need to unbox it.
            if (ret) {
                 res = *(Vector3*)((char*)ret + 0x10); // Unbox: skip header
            }
            return res;
        }

        Vector3 GetPosition(void* component) {
            if (!component) return {0,0,0};
            // Component.transform
            if (!g_ComponentClass) g_ComponentClass = GetClass("UnityEngine", "Component");
            
            static Il2CppMethod* getTrans = nullptr;
            if (!getTrans) getTrans = GetMethod((Il2CppClass*)g_ComponentClass, "get_transform", 0);
            
            if (!getTrans) return {0,0,0};
            
            void* transform = RuntimeInvoke(getTrans, component, nullptr, nullptr);
            return GetTransformPosition(transform);
        }

        Vector3 WorldToScreen(Vector3 worldPos) {
            void* cam = GetPlayerCamera();
            if (!cam) return {0,0,0};

            if (!g_CameraClass) g_CameraClass = GetClass("UnityEngine", "Camera");
            static Il2CppMethod* w2s = nullptr;
            if (!w2s) w2s = GetMethod((Il2CppClass*)g_CameraClass, "WorldToScreenPoint", 1);
            if (!w2s) return {0,0,0};

            void* args[1] = { &worldPos };
            void* ret = RuntimeInvoke(w2s, cam, args, nullptr);
            
            if (ret) {
                return *(Vector3*)((char*)ret + 0x10); // Unbox
            }
            return {0,0,0};
        }
    }
}
