#pragma once
// ============================================================
// il2cpp-api.h - IL2CPP 运行时 API 声明（标准 Unity IL2CPP 导出）
// 通过 GetProcAddress(GameAssembly.dll, "il2cpp_xxx") 按函数名加载
// 说明：SurvivalLog 是标准 IL2CPP 游戏（非魔改），GameAssembly.dll
//       会导出全套 il2cpp_* 函数，无需像原神那样手动定位 RVA。
// ============================================================
#include <Windows.h>
#include <cstddef>
#include <cstdint>

// ---------- 核心类型（仅需指针语义的最小声明） ----------
struct Il2CppClass;
struct Il2CppType;
struct Il2CppString;
struct MethodInfo;
struct FieldInfo;
struct Il2CppImage;
struct Il2CppAssembly;
struct Il2CppDomain;
struct Il2CppException;
struct Il2CppThread;

typedef int32_t il2cpp_array_size_t;

struct Il2CppObject
{
    Il2CppClass* klass; // +0x00
    void* monitor;      // +0x08
};

struct Il2CppArrayBounds
{
    il2cpp_array_size_t length;
    int32_t lower_bound;
};

struct Il2CppArray : Il2CppObject
{
    Il2CppArrayBounds* bounds;      // +0x10
    il2cpp_array_size_t max_length; // +0x18
    // 数据区从 +0x20 开始
};

// ---------- API 函数指针类型 ----------
typedef Il2CppDomain* (*il2cpp_domain_get_t)();
typedef Il2CppThread* (*il2cpp_thread_attach_t)(Il2CppDomain* domain);
typedef const Il2CppAssembly** (*il2cpp_domain_get_assemblies_t)(const Il2CppDomain* domain, size_t* size);
typedef Il2CppImage* (*il2cpp_assembly_get_image_t)(const Il2CppAssembly* assembly);
typedef const char* (*il2cpp_image_get_name_t)(Il2CppImage* image);
typedef Il2CppClass* (*il2cpp_class_from_name_t)(Il2CppImage* image, const char* namespaze, const char* name);
typedef Il2CppClass* (*il2cpp_class_get_parent_t)(Il2CppClass* klass);
typedef FieldInfo* (*il2cpp_class_get_fields_t)(Il2CppClass* klass, void** iter);
typedef const char* (*il2cpp_field_get_name_t)(FieldInfo* field);
typedef size_t (*il2cpp_field_get_offset_t)(FieldInfo* field);
typedef void (*il2cpp_field_static_get_value_t)(FieldInfo* field, void* value);
typedef void (*il2cpp_field_static_set_value_t)(FieldInfo* field, void* value);
typedef MethodInfo* (*il2cpp_class_get_method_from_name_t)(Il2CppClass* klass, const char* name, int argsCount);
typedef const MethodInfo* (*il2cpp_class_get_methods_t)(Il2CppClass* klass, void** iter);
typedef const char* (*il2cpp_method_get_name_t)(const MethodInfo* method);
typedef uint32_t (*il2cpp_method_get_param_count_t)(const MethodInfo* method);
typedef Il2CppObject* (*il2cpp_runtime_invoke_t)(MethodInfo* method, void* obj, void** params, Il2CppException** exc);
typedef Il2CppString* (*il2cpp_string_new_t)(const char* str);
typedef void* (*il2cpp_gchandle_new_t)(Il2CppObject* obj, bool pinned);
typedef Il2CppObject* (*il2cpp_gchandle_get_target_t)(void* handle);
typedef Il2CppClass* (*il2cpp_object_get_class_t)(Il2CppObject* obj);

// Il2CppType 的 type 枚举位（64 位布局：attrs@0x08, type@0x0A, num_mods/byref/pinned@0x0B, kind/valuetype@0x0C）
// 标准 IL2CPP_TYPE_* 常量（il2cpp-c-types.h）
enum
{
    IL2CPP_TYPE_VOID = 0x01,
    IL2CPP_TYPE_BOOLEAN = 0x02,
    IL2CPP_TYPE_I4 = 0x08,
    IL2CPP_TYPE_U4 = 0x09,
    IL2CPP_TYPE_I8 = 0x0A,
    IL2CPP_TYPE_U8 = 0x0B,
    IL2CPP_TYPE_R4 = 0x0C,
    IL2CPP_TYPE_R8 = 0x0D,
    IL2CPP_TYPE_STRING = 0x0E,
    IL2CPP_TYPE_CLASS = 0x12,
    IL2CPP_TYPE_OBJECT = 0x1C,
    IL2CPP_TYPE_ENUM = 0x55,
};

// ---------- 加载后的 API 集合 ----------
struct IL2CPP_API
{
    il2cpp_domain_get_t domain_get;
    il2cpp_thread_attach_t thread_attach;
    il2cpp_domain_get_assemblies_t domain_get_assemblies;
    il2cpp_assembly_get_image_t assembly_get_image;
    il2cpp_image_get_name_t image_get_name;
    il2cpp_class_from_name_t class_from_name;
    il2cpp_class_get_parent_t class_get_parent;
    il2cpp_class_get_fields_t class_get_fields;
    il2cpp_field_get_name_t field_get_name;
    il2cpp_field_get_offset_t field_get_offset;
    il2cpp_field_static_get_value_t field_static_get_value;
    il2cpp_field_static_set_value_t field_static_set_value;
    il2cpp_class_get_method_from_name_t class_get_method_from_name;
    il2cpp_class_get_methods_t class_get_methods;
    il2cpp_method_get_name_t method_get_name;
    il2cpp_method_get_param_count_t method_get_param_count;
    il2cpp_runtime_invoke_t runtime_invoke;
    il2cpp_string_new_t string_new;
    il2cpp_gchandle_new_t gchandle_new;
    il2cpp_gchandle_get_target_t gchandle_get_target;
    il2cpp_object_get_class_t object_get_class;
};

// 加载 GameAssembly.dll 的 il2cpp_* 导出（幂等）
bool LoadIL2CPPApi();

// 全局 API 集合（加载成功后可用）
extern IL2CPP_API* g_IL2CPP;
