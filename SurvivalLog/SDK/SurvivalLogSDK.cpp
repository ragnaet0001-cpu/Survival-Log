#include "SurvivalLogSDK.h"
#include "offsets.h"
#include "../DXhook/dev/logger.h"
#include <string.h>
#include <stdio.h>
#include <map>
#include <unordered_set>
#include <cmath>

// ============================================================
// SurvivalLogSDK.cpp - 实现
// 全部通过 GameAssembly.dll 导出的 il2cpp_* API 按函数名解析，
// 不依赖任何 RVA 硬编码（游戏更新后重新 dump SDK 即可）。
// ============================================================

IL2CPP_API* g_IL2CPP = nullptr;

// ---------- 加载 il2cpp 运行时 API ----------
bool LoadIL2CPPApi()
{
    if (g_IL2CPP)
        return true;

    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    if (!hGameAssembly)
    {
        LOG_ERROR("GameAssembly.dll not loaded");
        return false;
    }

    static IL2CPP_API api = {};
#define LOAD_API(member, name)                                          \
    api.member = (name##_t)GetProcAddress(hGameAssembly, #name);        \
    if (!api.member)                                                    \
    {                                                                   \
        LOG_ERROR("il2cpp export missing: %s", #name);                  \
        return false;                                                   \
    }

    LOAD_API(domain_get, il2cpp_domain_get)
    LOAD_API(thread_attach, il2cpp_thread_attach)
    LOAD_API(domain_get_assemblies, il2cpp_domain_get_assemblies)
    LOAD_API(assembly_get_image, il2cpp_assembly_get_image)
    LOAD_API(image_get_name, il2cpp_image_get_name)
    LOAD_API(class_from_name, il2cpp_class_from_name)
    LOAD_API(class_get_parent, il2cpp_class_get_parent)
    LOAD_API(class_get_fields, il2cpp_class_get_fields)
    LOAD_API(field_get_name, il2cpp_field_get_name)
    LOAD_API(field_static_get_value, il2cpp_field_static_get_value)
    LOAD_API(field_static_set_value, il2cpp_field_static_set_value)
    LOAD_API(class_get_method_from_name, il2cpp_class_get_method_from_name)
    LOAD_API(class_get_methods, il2cpp_class_get_methods)
    LOAD_API(method_get_name, il2cpp_method_get_name)
    LOAD_API(method_get_param_count, il2cpp_method_get_param_count)
    LOAD_API(runtime_invoke, il2cpp_runtime_invoke)
    LOAD_API(string_new, il2cpp_string_new)
    LOAD_API(gchandle_new, il2cpp_gchandle_new)
    LOAD_API(gchandle_get_target, il2cpp_gchandle_get_target)
    LOAD_API(object_get_class, il2cpp_object_get_class)
#undef LOAD_API

    g_IL2CPP = &api;
    LOG_INFO("IL2CPP API loaded");
    return true;
}

// ---------- 内部状态 ----------
namespace
{
    Il2CppDomain* g_domain = nullptr;
    Il2CppImage* g_hotUpdateImage = nullptr;

    Il2CppClass* g_klassBattleLogicWorld = nullptr;  // GameCore.HotUpdate.Battle.Logic
    Il2CppClass* g_klassAgentManager = nullptr;
    Il2CppClass* g_klassAttributeComponent = nullptr;
    Il2CppClass* g_klassAttr = nullptr;
    Il2CppClass* g_klassUserComponent = nullptr;
    Il2CppClass* g_klassReduxUISystem = nullptr;     // GameCore.HotUpdate.ReduxUI
    Il2CppClass* g_klassReduxStoreLayer = nullptr;
    Il2CppClass* g_klassStateDataPlayer = nullptr;
    Il2CppClass* g_klassStateWebCoreUI0 = nullptr;
    Il2CppClass* g_klassShopComponent = nullptr;     // GameCore.HotUpdate.Battle.Logic
    Il2CppClass* g_klassShopItem = nullptr;
    Il2CppClass* g_klassStateDataShop = nullptr;     // GameCore.HotUpdate.ReduxUI
    Il2CppClass* g_klassStateWebShopUI = nullptr;
    Il2CppClass* g_klassStateWebBuildShop = nullptr;
    Il2CppClass* g_klassStateWebFurnitureDetail = nullptr;
    Il2CppClass* g_klassGameTimeManager = nullptr;
    Il2CppClass* g_klassCountDownTimer = nullptr;
    Il2CppClass* g_klassLeadingRole = nullptr;
    Il2CppClass* g_klassLogicAdapter = nullptr;
    Il2CppClass* g_klassDataWebBuildShopProduct = nullptr;

    // 缓存的方法（本类方法，可直接 runtime_invoke）
    MethodInfo* g_miGetLeadingRole = nullptr;
    MethodInfo* g_miGetBaseValueInt = nullptr;
    MethodInfo* g_miSetBaseValueInt = nullptr;
    MethodInfo* g_miSyncAttr2UI = nullptr;
    MethodInfo* g_miAddGold = nullptr;
    MethodInfo* g_miSyncGold = nullptr;

    // ============ 动态字段偏移（Init 时从 il2cpp 元数据解析，游戏更新后自动适配，免更偏移） ============
    // 偏移变量本体在 offsets.h（全免更）：Init 时按字段名动态解析并直接填充 OFF_* 变量
    // ===== 批次2/3 动态解析字段（变量本体在 offsets.h，ModBatch2Init 时按字段名解析填充） =====


    // 沿继承链查字段偏移（字段名精确匹配，找不到返回 fallback）
    // HybridCLR 补充元数据类的字段遍历可能不完整，内部独立 __try 保护，失败降级 fallback
    size_t ResolveFieldOffset(Il2CppClass* klass, const char* fieldName, size_t fallback)
    {
        if (!klass || !g_IL2CPP)
            return fallback;
        __try
        {
            for (Il2CppClass* k = klass; k; k = g_IL2CPP->class_get_parent(k))
            {
                void* iter = nullptr;
                FieldInfo* f;
                while ((f = g_IL2CPP->class_get_fields(k, &iter)))
                {
                    const char* n = g_IL2CPP->field_get_name(f);
                    if (n && strcmp(n, fieldName) == 0)
                        return (size_t)g_IL2CPP->field_get_offset(f);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return fallback;
    }

    // 解析全部字段偏移（Init 成功后调用，只在首次解析）
    void ResolveAllOffsets()
    {
        OFF_AC_AttrDict = ResolveFieldOffset(g_klassAttributeComponent, "AttrDict", OFF_AC_AttrDict);
        OFF_ATTR_BaseValue = ResolveFieldOffset(g_klassAttr, "<BaseValue>k__BackingField", OFF_ATTR_BaseValue);
        OFF_ATTR_Max = ResolveFieldOffset(g_klassAttr, "<Max>k__BackingField", OFF_ATTR_Max);
        OFF_UC_Money = ResolveFieldOffset(g_klassUserComponent, "<Money>k__BackingField", OFF_UC_Money);
        OFF_BA_agentComponentDictionary = ResolveFieldOffset(g_klassLeadingRole, "agentComponentDictionary", OFF_BA_agentComponentDictionary);
        OFF_AM_agentList = ResolveFieldOffset(g_klassAgentManager, "<agentList>k__BackingField", OFF_AM_agentList);
        OFF_BLW_AgentManager = ResolveFieldOffset(g_klassBattleLogicWorld, "<_AgentManager>k__BackingField", OFF_BLW_AgentManager);
        OFF_BLW_GameTimeManager = ResolveFieldOffset(g_klassBattleLogicWorld, "<_GameTimeManager>k__BackingField", OFF_BLW_GameTimeManager);
        OFF_GTM_Timer = ResolveFieldOffset(g_klassGameTimeManager, "<Timer>k__BackingField", OFF_GTM_Timer);
        OFF_GTM_WorldStateType = ResolveFieldOffset(g_klassGameTimeManager, "<WorldStateType>k__BackingField", OFF_GTM_WorldStateType);
        OFF_CDT_RemainTime = ResolveFieldOffset(g_klassCountDownTimer, "<RemainTime>k__BackingField", OFF_CDT_RemainTime);
        OFF_SDP_Gold = ResolveFieldOffset(g_klassStateDataPlayer, "<Gold>k__BackingField", OFF_SDP_Gold);
        OFF_SDP_WorldState = ResolveFieldOffset(g_klassStateDataPlayer, "<WorldState>k__BackingField", OFF_SDP_WorldState);
        OFF_SWC_DisplaySeconds = ResolveFieldOffset(g_klassStateWebCoreUI0, "<DisplaySeconds>k__BackingField", OFF_SWC_DisplaySeconds);
        OFF_SWC_DecreasePerSecond = ResolveFieldOffset(g_klassStateWebCoreUI0, "<DecreasePerSecond>k__BackingField", OFF_SWC_DecreasePerSecond);
        OFF_SWB_Gold = ResolveFieldOffset(g_klassStateWebBuildShop, "<Gold>k__BackingField", OFF_SWB_Gold);
        OFF_SWB_PackageCapacity = ResolveFieldOffset(g_klassStateWebBuildShop, "<PackageCapacity>k__BackingField", OFF_SWB_PackageCapacity);
        OFF_SWB_Products = ResolveFieldOffset(g_klassStateWebBuildShop, "<Products>k__BackingField", OFF_SWB_Products);
        OFF_PRODUCT_SlotCurrent = ResolveFieldOffset(g_klassDataWebBuildShopProduct, "<SlotCurrent>k__BackingField", OFF_PRODUCT_SlotCurrent);
        OFF_SC_ShopCache = ResolveFieldOffset(g_klassShopComponent, "<ShopCache>k__BackingField", OFF_SC_ShopCache);
        OFF_SI_ItemCount = ResolveFieldOffset(g_klassShopItem, "<ItemCount>k__BackingField", OFF_SI_ItemCount);
        OFF_DATASHOP_ShopCache = ResolveFieldOffset(g_klassStateDataShop, "<ShopCache>k__BackingField", OFF_DATASHOP_ShopCache);
        OFF_SHOPUI_ShopCache = ResolveFieldOffset(g_klassStateWebShopUI, "<ShopCache>k__BackingField", OFF_SHOPUI_ShopCache);
        OFF_FD_CanBuy = ResolveFieldOffset(g_klassStateWebFurnitureDetail, "<CanBuy>k__BackingField", OFF_FD_CanBuy);
        OFF_FD_SoldOut = ResolveFieldOffset(g_klassStateWebFurnitureDetail, "<SoldOut>k__BackingField", OFF_FD_SoldOut);
        OFF_RUI_reduxStoreLayer = ResolveFieldOffset(g_klassReduxUISystem, "reduxStoreLayer", OFF_RUI_reduxStoreLayer);
        OFF_RUI_logicAdapter = ResolveFieldOffset(g_klassReduxUISystem, "_logicAdapter", OFF_RUI_logicAdapter);
        OFF_RSL_dataTree = ResolveFieldOffset(g_klassReduxStoreLayer, "dataTree", OFF_RSL_dataTree);
        OFF_RSL_cacheTree = ResolveFieldOffset(g_klassReduxStoreLayer, "cacheTree", OFF_RSL_cacheTree);
        OFF_RSL_stateTree = ResolveFieldOffset(g_klassReduxStoreLayer, "stateTree", OFF_RSL_stateTree);
        OFF_LA_currentWorldState = ResolveFieldOffset(g_klassLogicAdapter, "_currentWorldState", OFF_LA_currentWorldState);

        // 结构体偏移校验：动态解析值 vs 结构体编译期偏移，不一致警告提示更新结构体
        // （访问走结构体；这里只做检测，游戏更新后偏移变化会打印警告）
#define CHECK_STRUCT_OFF(klass, fieldName, structType, structField)                    \
        {                                                                              \
            size_t dyn = ResolveFieldOffset(klass, fieldName, (size_t)-1);             \
            size_t st = offsetof(structType, structField);                             \
            if (dyn != (size_t)-1 && dyn != st)                                        \
                LOG_WARN("Struct offset mismatch: %s dyn=0x%zX struct=0x%zX (update struct)", fieldName, dyn, st); \
        }
        CHECK_STRUCT_OFF(g_klassAttributeComponent, "AttrDict", AttributeComponent_o, AttrDict);
        CHECK_STRUCT_OFF(g_klassAttr, "<BaseValue>k__BackingField", Attr_o, BaseValue);
        CHECK_STRUCT_OFF(g_klassAttr, "<Max>k__BackingField", Attr_o, Max);
        CHECK_STRUCT_OFF(g_klassUserComponent, "<Money>k__BackingField", UserComponent_o, Money);
        CHECK_STRUCT_OFF(g_klassLeadingRole, "agentComponentDictionary", BaseAgent_o, agentComponentDictionary);
        CHECK_STRUCT_OFF(g_klassAgentManager, "<agentList>k__BackingField", AgentManager_o, _agentList);
        CHECK_STRUCT_OFF(g_klassBattleLogicWorld, "<_AgentManager>k__BackingField", BattleLogicWorld_o, _AgentManager);
        CHECK_STRUCT_OFF(g_klassBattleLogicWorld, "<_GameTimeManager>k__BackingField", BattleLogicWorld_o, _GameTimeManager);
        CHECK_STRUCT_OFF(g_klassGameTimeManager, "<Timer>k__BackingField", GameTimeManager_o, _Timer);
        CHECK_STRUCT_OFF(g_klassGameTimeManager, "<WorldStateType>k__BackingField", GameTimeManager_o, _WorldStateType);
        CHECK_STRUCT_OFF(g_klassCountDownTimer, "<RemainTime>k__BackingField", CountDownTimer_o, _RemainTime);
        CHECK_STRUCT_OFF(g_klassStateDataPlayer, "<Gold>k__BackingField", State_Data_Player_o, _Gold);
        CHECK_STRUCT_OFF(g_klassStateDataPlayer, "<WorldState>k__BackingField", State_Data_Player_o, _WorldState);
        CHECK_STRUCT_OFF(g_klassStateWebCoreUI0, "<DisplaySeconds>k__BackingField", State_Web_CoreUI0_o, _DisplaySeconds);
        CHECK_STRUCT_OFF(g_klassStateWebCoreUI0, "<DecreasePerSecond>k__BackingField", State_Web_CoreUI0_o, _DecreasePerSecond);
        CHECK_STRUCT_OFF(g_klassStateWebBuildShop, "<Gold>k__BackingField", State_Web_BuildShop_o, Gold);
        CHECK_STRUCT_OFF(g_klassStateWebBuildShop, "<PackageCapacity>k__BackingField", State_Web_BuildShop_o, PackageCapacity);
        CHECK_STRUCT_OFF(g_klassStateWebBuildShop, "<Products>k__BackingField", State_Web_BuildShop_o, Products);
        CHECK_STRUCT_OFF(g_klassDataWebBuildShopProduct, "<SlotCurrent>k__BackingField", Data_Web_BuildShop_Product_o, SlotCurrent);
        CHECK_STRUCT_OFF(g_klassShopComponent, "<ShopCache>k__BackingField", ShopComponent_o, ShopCache);
        CHECK_STRUCT_OFF(g_klassShopItem, "<ItemCount>k__BackingField", ShopItem_o, ItemCount);
        CHECK_STRUCT_OFF(g_klassStateDataShop, "<ShopCache>k__BackingField", State_Data_Shop_o, _ShopCache);
        CHECK_STRUCT_OFF(g_klassStateWebShopUI, "<ShopCache>k__BackingField", State_Web_ShopUI_o, _ShopCache);
        CHECK_STRUCT_OFF(g_klassReduxUISystem, "reduxStoreLayer", ReduxUISystem_o, reduxStoreLayer);
        CHECK_STRUCT_OFF(g_klassReduxUISystem, "_logicAdapter", ReduxUISystem_o, _logicAdapter);
        CHECK_STRUCT_OFF(g_klassReduxStoreLayer, "dataTree", ReduxStoreLayer_o, dataTree);
        CHECK_STRUCT_OFF(g_klassReduxStoreLayer, "cacheTree", ReduxStoreLayer_o, cacheTree);
        CHECK_STRUCT_OFF(g_klassReduxStoreLayer, "stateTree", ReduxStoreLayer_o, stateTree);
#undef CHECK_STRUCT_OFF

        LOG_INFO("Field offsets resolved (dynamic)");
    }
}

// 从继承链找方法（IL2CPP class_get_method_from_name 只查本类，需手动沿父类找）
static MethodInfo* FindMethodInHierarchy(Il2CppClass* klass, const char* name, int argsCount)
{
    for (Il2CppClass* k = klass; k; k = g_IL2CPP->class_get_parent(k))
    {
        MethodInfo* m = g_IL2CPP->class_get_method_from_name(k, name, argsCount);
        if (m)
            return m;
    }
    return nullptr;
}

// 调用静态方法 get_Instance（BaseSingleton<T>），返回实例
static Il2CppObject* GetSingletonInstance(Il2CppClass* klass)
{
    if (!klass || !g_IL2CPP)
        return nullptr;
    MethodInfo* mi = FindMethodInHierarchy(klass, "get_Instance", 0);
   // LOG_INFO("g_miGetInstance:%p", mi);
    if (!mi)
        return nullptr;
    Il2CppException* exc = nullptr;
    return g_IL2CPP->runtime_invoke(mi, nullptr, nullptr, &exc);
}

// 调实例方法（无参）
static Il2CppObject* InvokeNoArg(MethodInfo* mi, void* obj)
{
    if (!mi || !g_IL2CPP)
        return nullptr;
    Il2CppException* exc = nullptr;
    return g_IL2CPP->runtime_invoke(mi, obj, nullptr, &exc);
}

// 调实例方法（一个 int 参数）
static Il2CppObject* InvokeIntArg(MethodInfo* mi, void* obj, int32_t arg)
{
    if (!mi || !g_IL2CPP)
        return nullptr;
    void* params[1] = { &arg };
    Il2CppException* exc = nullptr;
    return g_IL2CPP->runtime_invoke(mi, obj, params, &exc);
}

// 调实例方法（一个 int 参数），返回是否无异常（void 方法 runtime_invoke 返回 nullptr 但 exc==nullptr 即成功）
static bool InvokeIntArgOk(MethodInfo* mi, void* obj, int32_t arg)
{
    if (!mi || !obj || !g_IL2CPP)
        return false;
    void* params[1] = { &arg };
    Il2CppException* exc = nullptr;
    g_IL2CPP->runtime_invoke(mi, obj, params, &exc);
    return exc == nullptr;
}

// 字典遍历：按 value 的 klass 匹配（Dictionary<TKey, TValue>，Entry 24 字节）
static Il2CppObject* DictFindByKlass(void* dict, Il2CppClass* targetKlass)
{
    if (!dict || !targetKlass)
        return nullptr;
    Dictionary_o* d = (Dictionary_o*)dict;
    if (!d->_entries || d->_count <= 0 || d->_count > 100000)
        return nullptr;
    uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA; // 数组数据区
    for (int32_t i = 0; i < d->_count; i++)
    {
        uint8_t* entry = entries + (size_t)i * 24;
        void* value = *(void**)(entry + 16); // Entry: hashCode(4) next(4) key(8) value(8)
        if (value && ((Il2CppObject*)value)->klass == targetKlass)
            return (Il2CppObject*)value;
    }
    return nullptr;
}

// 字典遍历：按 int key 匹配（AttrName -> Attr）
static Attr_o* DictFindAttr(void* dict, int32_t key, Il2CppClass* attrKlass)
{
    if (!dict || !attrKlass)
        return nullptr;
    Dictionary_o* d = (Dictionary_o*)dict;
    if (!d->_entries || d->_count <= 0 || d->_count > 100000)
        return nullptr;
    uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
    for (int32_t i = 0; i < d->_count; i++)
    {
        uint8_t* entry = entries + (size_t)i * 24;
        int32_t k = *(int32_t*)(entry + 8);      // AttrName (int32)
        void* value = *(void**)(entry + 16);
        if (k == key && value && ((Il2CppObject*)value)->klass == attrKlass)
            return (Attr_o*)value;
    }
    return nullptr;
}

// 字典按 int key 找 value（Dictionary<int, T>，entry 24 字节；key 在 entry+8，value 在 entry+16）
static void* DictFindByIntKey(void* dict, int32_t key)
{
    if (!dict)
        return nullptr;
    Dictionary_o* d = (Dictionary_o*)dict;
    if (!d->_entries || d->_count <= 0 || d->_count > 100000)
        return nullptr;
    uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
    for (int32_t i = 0; i < d->_count; i++)
    {
        uint8_t* entry = entries + (size_t)i * 24;
        if (*(int32_t*)(entry + 8) == key)
            return *(void**)(entry + 16);
    }
    return nullptr;
}

// ---------- IL2CPP List<T> 辅助 ----------
// List 字段布局（il2cpp.h System_Collections_Generic_List_*__Fields）：
//   +0x10 _items(数组引用), +0x18 _size(int32) = Count, +0x1C _version
static size_t ListGetCount(void* list)
{
    if (!list)
        return 0;
    return (size_t)(*(int32_t*)((uint8_t*)list + OFF_LIST_SIZE)); // List._size
}

static void* ListGetItem(void* list, size_t index)
{
    if (!list)
        return nullptr;
    void* items = *(void**)((uint8_t*)list + OFF_LIST_ITEMS); // _items 数组
    if (!items)
        return nullptr;
    // 引用类型数组：每元素 8 字节指针，解引用拿对象
    return *(void**)((uint8_t*)items + OFF_ARRAY_DATA + index * 8);
}

// ---------- 初始化 ----------
bool SLSDK_Init()
{
    if (!LoadIL2CPPApi())
        return false;

    if (g_hotUpdateImage && g_klassAttr)
        return true; // 已就绪

    __try
    {
        if (!g_domain)
        {
            g_domain = g_IL2CPP->domain_get();
            if (!g_domain)
                return false;
            g_IL2CPP->thread_attach(g_domain);
        }

        // 找 HotUpdate.dll（HybridCLR 热更新主程序集）
        if (!g_hotUpdateImage)
        {
            size_t size = 0;
            const Il2CppAssembly** assemblies = g_IL2CPP->domain_get_assemblies(g_domain, &size);
            for (size_t i = 0; i < size; i++)
            {
                Il2CppImage* img = g_IL2CPP->assembly_get_image(assemblies[i]);
                if (img)
                {
                    const char* n = g_IL2CPP->image_get_name(img);
                    if (n && strcmp(n, "HotUpdate.dll") == 0)
                    {
                        g_hotUpdateImage = img;
                        break;
                    }
                }
            }
            if (!g_hotUpdateImage)
                return false; // 热更新程序集未加载，稍后重试
            LOG_INFO("HotUpdate.dll image found");
        }

        // 拿各类 klass
        g_klassBattleLogicWorld = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "BattleLogicWorld");
        LOG_INFO("g_klassBattleLogicWorld:%p", g_klassBattleLogicWorld);    
        g_klassAgentManager     = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "AgentManager");
        g_klassAttributeComponent = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "AttributeComponent");
        g_klassAttr             = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "Attr");
        g_klassUserComponent    = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "UserComponent");
        g_klassReduxUISystem    = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "ReduxUISystem");
        g_klassReduxStoreLayer  = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "ReduxStoreLayer");
        g_klassStateDataPlayer  = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "State_Data_Player");
        g_klassStateWebCoreUI0  = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "State_Web_CoreUI0");
        g_klassShopComponent    = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "ShopComponent");
        g_klassShopItem         = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "ShopItem");
        g_klassGameTimeManager  = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "GameTimeManager");
        g_klassCountDownTimer   = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "CountDownTimer");
        g_klassLeadingRole      = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "LeadingRole");
        g_klassLogicAdapter     = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "LogicAdapter");
        g_klassDataWebBuildShopProduct = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "Data_Web_BuildShop_Product");
        g_klassStateDataShop    = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "State_Data_Shop");
        g_klassStateWebShopUI   = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "State_Web_ShopUI");
        g_klassStateWebBuildShop = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "State_Web_BuildShop");
        g_klassStateWebFurnitureDetail = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "State_Web_FurnitureDetail");

        if (!g_klassBattleLogicWorld || !g_klassAgentManager || !g_klassAttributeComponent ||
            !g_klassAttr || !g_klassUserComponent || !g_klassReduxUISystem || !g_klassReduxStoreLayer ||
            !g_klassStateDataPlayer || !g_klassStateWebCoreUI0)
        {
            LOG_ERROR("SDK class resolve failed");
            return false;
        }

        // 缓存方法
        g_miGetLeadingRole = g_IL2CPP->class_get_method_from_name(g_klassAgentManager, "GetLeadingRole", 0);
        g_miGetBaseValueInt = g_IL2CPP->class_get_method_from_name(g_klassAttributeComponent, "GetBaseValue_Int", 1);
        g_miSetBaseValueInt = g_IL2CPP->class_get_method_from_name(g_klassAttributeComponent, "SetBaseValue_Int", 2);
        g_miSyncAttr2UI = g_IL2CPP->class_get_method_from_name(g_klassAttributeComponent, "SyncAttr2UI", 1);
        g_miAddGold = FindMethodInHierarchy(g_klassLeadingRole, "AddGold", 1);
        g_miSyncGold = FindMethodInHierarchy(g_klassLeadingRole, "SyncGold", 1);
        LOG_INFO("g_miAddGold:%p", g_miAddGold);
        LOG_INFO("g_miSyncGold:%p", g_miSyncGold);
        LOG_INFO("SurvivalLog SDK ready (BattleLogicWorld/ReduxUISystem resolved)");

        // 解析动态字段偏移（失败降级用 fallback，不影响 SDK 使用）
        __try
        {
            ResolveAllOffsets();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ResolveAllOffsets exception 0x%08X", GetExceptionCode());
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        LOG_ERROR("SLSDK_Init exception 0x%08X", GetExceptionCode());
        return false;
    }
}

bool SLSDK_Ready()
{
    return g_hotUpdateImage && g_klassAttr && g_IL2CPP;
}


// ---------- 实例获取 ----------

// BattleLogicWorld.Instance -> LeadingRole（主角）
static LeadingRole_o* GetLeadingRole()
{
    __try
    {
        BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
        if (!world || !world->_AgentManager)
            return nullptr;
        AgentManager_o* mgr = world->_AgentManager;
        return (LeadingRole_o*)InvokeNoArg(g_miGetLeadingRole, mgr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

// 从 LeadingRole 的 agentComponentDictionary 找指定类型组件
static Il2CppObject* GetLeadingRoleComponent(Il2CppClass* compKlass)
{
    __try
    {
        LeadingRole_o* leading = GetLeadingRole();
        if (!leading || !leading->agentComponentDictionary)
            return nullptr;
        return DictFindByKlass(leading->agentComponentDictionary, compKlass);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

// BattleLogicWorld.Instance -> AttributeComponent（玩家属性组件）
static AttributeComponent_o* GetAttributeComponent()
{
    return (AttributeComponent_o*)GetLeadingRoleComponent(g_klassAttributeComponent);
}

// BattleLogicWorld.Instance -> UserComponent（金币真源）
static UserComponent_o* GetUserComponent()
{
    return (UserComponent_o*)GetLeadingRoleComponent(g_klassUserComponent);
}

// ReduxUISystem.Instance -> State_Data_Player / State_Web_CoreUI0
// 注意：Redux 有三棵树 dataTree/cacheTree/stateTree，
// State_Data_* 在 dataTree，State_Web_* 在 stateTree，全找一遍最稳
static Il2CppObject* GetReduxState(Il2CppClass* stateKlass)
{
    __try
    {
        ReduxUISystem_o* ui = (ReduxUISystem_o*)GetSingletonInstance(g_klassReduxUISystem);
        if (!ui || !ui->reduxStoreLayer)
            return nullptr;
        ReduxStoreLayer_o* store = ui->reduxStoreLayer;
        Il2CppObject* obj = DictFindByKlass(store->dataTree, stateKlass);
        if (obj)
            return obj;
        obj = DictFindByKlass(store->stateTree, stateKlass);
        if (obj)
            return obj;
        return DictFindByKlass(store->cacheTree, stateKlass);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

// ---------- 玩家属性（mod 完整版见文件末尾批次1：GetTotalValue/上限联动） ----------
// ---------- 金币 ----------
// 真源 = UserComponent.Money (+0x50)；显示层 = State_Data_Player._Gold (+0x2C)
int32_t SLSDK_GetGold()
{
    __try
    {
        UserComponent_o* uc = GetUserComponent();
        if (uc)
            return uc->Money;
        State_Data_Player_o* sp = (State_Data_Player_o*)GetReduxState(g_klassStateDataPlayer);
        return sp ? sp->_Gold : -1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

bool SLSDK_SetGold(int32_t value)
{
    __try
    {
        // 1. 优先游戏 API：LeadingRole.SyncGold(int)（mod SetMoney 同款，走游戏逻辑同步显示层/事件）
        if (g_miSyncGold)
        {
            LeadingRole_o* leading = GetLeadingRole();
            if (leading && InvokeIntArgOk(g_miSyncGold, leading, value))
                return true;
        }
        // 2. fallback：直写真源 + 显示层
        UserComponent_o* uc = GetUserComponent();
        if (!uc)
            return false;
        uc->Money = value;
        State_Data_Player_o* sp = (State_Data_Player_o*)GetReduxState(g_klassStateDataPlayer);
        if (sp)
            sp->_Gold = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_AddGold(int32_t amount)
{
    __try
    {
        if (amount <= 0)
            return false;
        // 与 mod AddGold 一致：读真源，截断到 int32 上限防溢出
        UserComponent_o* uc = GetUserComponent();
        int32_t cur = uc ? uc->Money : 0;
        int32_t capped = (amount > INT32_MAX - cur) ? (INT32_MAX - cur) : amount;
        if (capped <= 0)
            return false;
        // 1. 优先游戏 API：LeadingRole.AddGold(int)
        if (g_miAddGold)
        {
            LeadingRole_o* leading = GetLeadingRole();
            if (leading && InvokeIntArgOk(g_miAddGold, leading, capped))
                return true;
        }
        // 2. fallback：直写真源 + 显示层
        if (uc)
        {
            uc->Money = cur + capped;
            State_Data_Player_o* sp = (State_Data_Player_o*)GetReduxState(g_klassStateDataPlayer);
            if (sp)
                sp->_Gold = uc->Money;
            return true;
        }
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- 灾变倒计时 ----------
// 调 ReactiveProperty<T>.set_Value（从实例 klass 动态找方法，虚方法 runtime_invoke 自动分派）
// BattleLogicWorld.Instance -> _GameTimeManager -> _Timer -> CountDownTimer
// ---------- 灾变阶段（WorldStateType: 1=PreDisaster, 2=PostDisaster） ----------
// ---------- 商店库存恢复 ----------
// 遍历 AgentManager.agentList（商店家具也是 BaseAgent），找 ShopComponent
// 恢复 List<ShopItem>/List<Data_ShopItem> 里的 ItemCount
// ---------- 建造商店（State_Web_BuildShop） ----------
// ---------- 诊断信息（必须在所有 static helper 定义之后） ----------
void SLSDK_DebugInfo(char* buf, size_t len)
{
    if (!buf || len == 0)
        return;
    buf[0] = 0;
    __try
    {
        int off = 0;
        off += snprintf(buf + off, len - off, "SDK ready: %d\n", SLSDK_Ready() ? 1 : 0);
        BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
        off += snprintf(buf + off, len - off, "BattleLogicWorld: %p\n", world);
        if (world)
        {
            off += snprintf(buf + off, len - off, "  AgentManager: %p\n", world->_AgentManager);
            off += snprintf(buf + off, len - off, "  GameTimeManager: %p\n", world->_GameTimeManager);
        }
        ReduxUISystem_o* ui = (ReduxUISystem_o*)GetSingletonInstance(g_klassReduxUISystem);
        off += snprintf(buf + off, len - off, "ReduxUISystem: %p\n", ui);
        if (ui)
        {
            off += snprintf(buf + off, len - off, "  reduxStoreLayer: %p\n", ui->reduxStoreLayer);
            if (ui->reduxStoreLayer)
            {
                ReduxStoreLayer_o* store = ui->reduxStoreLayer;
                int32_t dc = 0, cc = 0, sc = 0;
                if (store->dataTree)
                    dc = ((Dictionary_o*)store->dataTree)->_count;
                if (store->cacheTree)
                    cc = ((Dictionary_o*)store->cacheTree)->_count;
                if (store->stateTree)
                    sc = ((Dictionary_o*)store->stateTree)->_count;
                off += snprintf(buf + off, len - off, "  dataTree=%d cacheTree=%d stateTree=%d\n", dc, cc, sc);
            }
        }
        AttributeComponent_o* comp = GetAttributeComponent();
        off += snprintf(buf + off, len - off, "AttributeComponent: %p\n", comp);
        Il2CppObject* sp = GetReduxState(g_klassStateDataPlayer);
        off += snprintf(buf + off, len - off, "State_Data_Player: %p\n", sp);
        Il2CppObject* cu = GetReduxState(g_klassStateWebCoreUI0);
        off += snprintf(buf + off, len - off, "State_Web_CoreUI0: %p\n", cu);
        State_Web_BuildShop_o* bs = (State_Web_BuildShop_o*)GetReduxState(g_klassStateWebBuildShop);
        off += snprintf(buf + off, len - off, "State_Web_BuildShop(当前商店): %p\n", bs);
        // ShopComponent 已随旧功能删除
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

#include <limits.h>
#include <vector>

// ============================================================
// 批次1：物品/背包 + 属性完整版（mod GameApi 迁移）
// 调用链：BattleLogicWorld._ItemManager(+0x78) / BaseSingleton<ConfigManager>.Instance
// 全部按方法名+参数个数解析，失败降级返回 false（不崩）
// ============================================================

// ---------- 值类型结构 ----------
struct Vector2Int_o
{
    int32_t x; // +0x00
    int32_t y; // +0x04
};

// Il2CppString（string）：length +0x10，UTF-16 数据从 +0x18 起
struct Il2CppString_o : Il2CppObject
{
    int32_t length;  // +0x10
    wchar_t chars[1]; // +0x18（64 位下 +0x14 为对齐填充）
};

// ItemData（BaseEntity 之后）
struct ItemData_o : BaseEntity_o
{
    int64_t OwnerId;      // +0x30
    int32_t ItemConfigId; // +0x38
    int32_t ItemCount;    // +0x3C
    int32_t StartTime;    // +0x40
    int32_t TimeLeft;     // +0x44
    Vector2Int_o ItemSize; // +0x48
    Vector2Int_o BagPos;   // +0x50
    int32_t UseTimes;     // +0x58
    int32_t MaxUseTimes;  // +0x5C
    float TimeScale;      // +0x60
    int32_t OriginalTimeLeft; // +0x64
    float TimeLeftFrac;   // +0x68
    Vector2Int_o OrigBagPos; // +0x6C
    void* InstanceVD;     // +0x78
    void* InstanceEffectEnd; // +0x80
    int32_t InstanceBurnValue; // +0x88
    bool Polluted;        // +0x8C
    bool IsMapPreset;     // +0x8D
    bool NoPackage;       // +0x8E
    float PresetShelfScale; // +0x90
};

// Config_Item（配置表条目，字段偏移来自 SDK dump）
struct Config_Item_o : Il2CppObject
{
    int32_t ID;               // +0x10
    void* ItemName;           // +0x18
    void* ItemName_Local;     // +0x20
    void* ItemDes1;           // +0x28
    void* ItemDes1_Local;     // +0x30
    void* ItemDes2;           // +0x38
    void* ItemDes2_Local;     // +0x40
    float ValueDisplay1;      // +0x48
    float ValueDisplay2;      // +0x4C
    float ValueDisplay3;      // +0x50
    float ValueDisplay4;      // +0x54
    float ValueDisplay5;      // +0x58
    int32_t StackLimit;       // +0x5C
    int32_t Category;         // +0x60
    int32_t SubCategory;      // +0x64
    bool CanCook;             // +0x68
    int32_t FoodTag1;         // +0x6C
    int32_t FoodTag2;         // +0x70
    int32_t FoodTag3;         // +0x74
    void* Size;               // +0x78 List<int>
    int32_t price;            // +0x80
    int32_t weight;           // +0x84
    int32_t Life;             // +0x88
};

// Config_Player
struct Config_Player_o : Il2CppObject
{
    int32_t ID;           // +0x10
    void* Name;           // +0x18
    void* Name_Local;     // +0x20
    void* Player;         // +0x28
    int32_t ClassType;    // +0x30
    void* SpineName;      // +0x38
    float Scale;          // +0x40
    void* NpcPic;         // +0x48
    void* ChapterID;      // +0x50
    void* LeadingRoleID;  // +0x58
    int32_t Bag;          // +0x60
    int32_t LinkAttr;     // +0x64
    void* BuffList;       // +0x68
    void* ColliderArgs;   // +0x70
    void* LogicConfig;    // +0x78
    void* TargetDecision; // +0x80
    void* DeadFmod;       // +0x88
    void* RunFmod;        // +0x90
    void* Dialogue;       // +0x98
    void* Dialogue_EN;    // +0xA0
    void* InitMoney;      // +0xA8
};

// Config_PlayAttribute（字段完整，保证 MoveSpeed 对齐 +0x78）
struct Config_PlayAttribute_o : Il2CppObject
{
    int32_t ID;          // +0x10
    void* Name;          // +0x18
    void* Name_Local;    // +0x20
    float Satiety;       // +0x28
    float Morale;        // +0x2C
    float Stamina;       // +0x30
    float Health;        // +0x34
    float Vitality;      // +0x38
    int32_t MaxSatiety;  // +0x3C
    int32_t MaxMorale;   // +0x40
    int32_t MaxStamina;  // +0x44
    int32_t MaxHealth;   // +0x48
    int32_t MaxVitality; // +0x4C
    int32_t IntervalSatiety;  // +0x50
    int32_t IntervalMorale;   // +0x54
    int32_t IntervalStamina;  // +0x58
    int32_t IntervalHealth;   // +0x5C
    int32_t IntervalVitality; // +0x60
    int32_t TargetSatiety;    // +0x64
    int32_t TargetMorale;     // +0x68
    int32_t TargetStamina;    // +0x6C
    int32_t TargetHealth;     // +0x70
    int32_t TargetVitality;   // +0x74
    float MoveSpeed;          // +0x78
};

// BagComponent（BaseComponent 之后）
struct BagComponent_o : BaseComponent_o
{
    int32_t BagWeight;  // +0x40
    int32_t Burden;     // +0x44
    int32_t BagConfigId; // +0x48
    int32_t InitialBagConfigId; // +0x4C
    int32_t ExtraBurden; // +0x50
    float OverweightCheckTime; // +0x54
};

// CountUpTimer（准备阶段以外的时间器）
struct CountUpTimer_o : Il2CppObject
{
    float CurrentTime; // +0x10
    int32_t MaxTime;   // +0x14
    int32_t Day;       // +0x18
    int32_t LastHour;  // +0x1C
    int32_t LastTenMinute; // +0x20
    int32_t DayEndStage;   // +0x24
};

namespace
{
    Il2CppClass* g_klassItemManager = nullptr;
    Il2CppClass* g_klassItemData = nullptr;
    Il2CppClass* g_klassConfigItem = nullptr;
    Il2CppClass* g_klassConfigPlayer = nullptr;
    Il2CppClass* g_klassConfigPlayAttribute = nullptr;
    Il2CppClass* g_klassConfigManager = nullptr;
    Il2CppClass* g_klassBagComponent = nullptr;
    Il2CppClass* g_klassCountUpTimer = nullptr;
    Il2CppClass* g_klassGameKey = nullptr;

    MethodInfo* g_miConfigGetItem = nullptr;       // Get_Config_Item(int)
    MethodInfo* g_miConfigGetPlayer = nullptr;     // Get_Config_Player(int)
    MethodInfo* g_miConfigGetPlayAttr = nullptr;   // Get_Config_PlayAttribute(int)
    MethodInfo* g_miItemAddItem = nullptr;         // AddItem(long,int,int,int,int,Vector2Int,bool,bool) 8参
    MethodInfo* g_miItemTryResolveBagPos = nullptr; // TryResolveBagPos(long,Vector2Int,Vector2Int,out Vector2Int) 4参
    MethodInfo* g_miItemGetItemDataList = nullptr; // GetItemDataList(long) 1参
    MethodInfo* g_miItemGetItemData = nullptr;     // GetItemData(long) 1参
    MethodInfo* g_miItemSetItemCount = nullptr;    // SetItemCount(ItemData,int) 2参
    MethodInfo* g_miItemForceRemoveItem = nullptr; // ForceRemoveItem(long) 1参
    MethodInfo* g_miGetTotalValueFloat = nullptr;  // GetTotalValue_Float(AttrName) 1参
    MethodInfo* g_miGetTotalValueInt = nullptr;    // GetTotalValue_Int(AttrName) 1参
    MethodInfo* g_miGetBaseValueFloat = nullptr;   // GetBaseValue_Float(AttrName) 1参
    MethodInfo* g_miSetBaseValueFloat = nullptr;   // SetBaseValue_Float(AttrName,float,bool) 3参
    bool g_modItemsInited = false;

    // 调实例方法并返回结果（不检查异常）
    static Il2CppObject* InvokeRet(MethodInfo* mi, void* obj, void** params)
    {
        if (!mi || !g_IL2CPP)
            return nullptr;
        Il2CppException* exc = nullptr;
        return g_IL2CPP->runtime_invoke(mi, obj, params, &exc);
    }

    // 调实例方法（任意参数），返回是否无异常
    static bool InvokeOk(MethodInfo* mi, void* obj, void** params)
    {
        if (!mi || !obj || !g_IL2CPP)
            return false;
        Il2CppException* exc = nullptr;
        g_IL2CPP->runtime_invoke(mi, obj, params, &exc);
        return exc == nullptr;
    }

    // 值类型返回值是装箱对象，数据在 +0x10
    static int32_t UnboxInt32(Il2CppObject* boxed, int32_t def)
    {
        if (!boxed)
            return def;
        __try { return *(int32_t*)((uint8_t*)boxed + OFF_BOXED_VALUE); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return def; }
    }
    static float UnboxFloat(Il2CppObject* boxed, float def)
    {
        if (!boxed)
            return def;
        __try { return *(float*)((uint8_t*)boxed + OFF_BOXED_VALUE); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return def; }
    }
    static bool UnboxBool(Il2CppObject* boxed, bool def)
    {
        if (!boxed)
            return def;
        __try { return *(uint8_t*)((uint8_t*)boxed + OFF_BOXED_VALUE) != 0; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return def; }
    }

    // Il2CppString -> UTF-8（截断安全）
    static void ILStringToUtf8(void* str, char* buf, size_t len)
    {
        if (!buf || len == 0)
            return;
        buf[0] = 0;
        if (!str)
            return;
        __try
        {
            Il2CppString_o* s = (Il2CppString_o*)str;
            int32_t n = s->length;
            if (n < 0)
                n = 0;
            if (n > 4096)
                n = 4096;
            int wlen = WideCharToMultiByte(CP_UTF8, 0, s->chars, n, buf, (int)len - 1, nullptr, nullptr);
            if (wlen > 0)
                buf[wlen] = 0;
            else
                buf[0] = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            buf[0] = 0;
        }
    }

    // List<int> 读取（_items 是 Int32[]，数据区每元素 4 字节）
    static int32_t IntListGet(void* list, size_t index, int32_t def)
    {
        if (!list)
            return def;
        __try
        {
            void* items = *(void**)((uint8_t*)list + OFF_LIST_ITEMS);
            if (!items)
                return def;
            return *(int32_t*)((uint8_t*)items + OFF_ARRAY_DATA + index * 4);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return def;
        }
    }

    // 查找静态字段（按名字）
    static FieldInfo* FindStaticField(Il2CppClass* klass, const char* name)
    {
        if (!klass || !g_IL2CPP)
            return nullptr;
        __try
        {
            void* iter = nullptr;
            FieldInfo* f;
            while ((f = g_IL2CPP->class_get_fields(klass, &iter)))
            {
                const char* n = g_IL2CPP->field_get_name(f);
                if (n && strcmp(n, name) == 0)
                    return f;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return nullptr;
    }

    // 读静态 float 字段
    static float GetStaticFloat(Il2CppClass* klass, const char* fieldName, float def)
    {
        if (!klass || !g_IL2CPP)
            return def;
        __try
        {
            FieldInfo* f = FindStaticField(klass, fieldName);
            if (!f)
                return def;
            float v = def;
            g_IL2CPP->field_static_get_value(f, &v);
            return v;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return def;
        }
    }

    // 懒初始化（panel 调用 API 时自动触发，不依赖 SLSDK_Init 时序）
    static bool ModItemsInit()
    {
        if (g_modItemsInited)
            return true;
        if (!SLSDK_Ready() || !g_IL2CPP)
            return false;
        __try
        {
            if (!g_hotUpdateImage)
                return false;
            g_klassItemManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "ItemManager");
            g_klassItemData = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "ItemData");
            g_klassConfigItem = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_Item");
            g_klassConfigPlayer = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_Player");
            g_klassConfigPlayAttribute = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_PlayAttribute");
            g_klassConfigManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "ConfigManager");
            g_klassBagComponent = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "BagComponent");
            g_klassCountUpTimer = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "CountUpTimer");
            g_klassGameKey = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "GameKey");
            if (!g_klassItemManager || !g_klassItemData || !g_klassConfigItem || !g_klassConfigManager)
                return false;

            g_miConfigGetItem = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "Get_Config_Item", 1);
            g_miConfigGetPlayer = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "Get_Config_Player", 1);
            g_miConfigGetPlayAttr = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "Get_Config_PlayAttribute", 1);
            g_miItemAddItem = g_IL2CPP->class_get_method_from_name(g_klassItemManager, "AddItem", 8);
            g_miItemTryResolveBagPos = g_IL2CPP->class_get_method_from_name(g_klassItemManager, "TryResolveBagPos", 4);
            g_miItemGetItemDataList = g_IL2CPP->class_get_method_from_name(g_klassItemManager, "GetItemDataList", 1);
            g_miItemGetItemData = g_IL2CPP->class_get_method_from_name(g_klassItemManager, "GetItemData", 1);
            g_miItemSetItemCount = g_IL2CPP->class_get_method_from_name(g_klassItemManager, "SetItemCount", 2);
            g_miItemForceRemoveItem = g_IL2CPP->class_get_method_from_name(g_klassItemManager, "ForceRemoveItem", 1);
            g_miGetTotalValueFloat = FindMethodInHierarchy(g_klassAttributeComponent, "GetTotalValue_Float", 1);
            g_miGetTotalValueInt = FindMethodInHierarchy(g_klassAttributeComponent, "GetTotalValue_Int", 1);
            g_miGetBaseValueFloat = FindMethodInHierarchy(g_klassAttributeComponent, "GetBaseValue_Float", 1);
            g_miSetBaseValueFloat = FindMethodInHierarchy(g_klassAttributeComponent, "SetBaseValue_Float", 3);

            // 字段偏移动态解析（免更偏移；失败保持 offsets.h fallback）
            OFF_IM_Cache = ResolveFieldOffset(g_klassItemManager, "<Cache>k__BackingField", OFF_IM_Cache);
            OFF_CM_ItemDict = ResolveFieldOffset(g_klassConfigManager, "_Config_Item_Dict", OFF_CM_ItemDict);
            OFF_CM_BuffDict = ResolveFieldOffset(g_klassConfigManager, "_Config_Buff_Dict", OFF_CM_BuffDict);
            OFF_CM_DailyRandomDict = ResolveFieldOffset(g_klassConfigManager, "_Config_DailyRandom_Dict", OFF_CM_DailyRandomDict);
            OFF_CM_RandomGroupDict = ResolveFieldOffset(g_klassConfigManager, "_Config_RandomGroup_Dict", OFF_CM_RandomGroupDict);
            OFF_CM_TalentDict = ResolveFieldOffset(g_klassConfigManager, "_Config_Talent_Dict", OFF_CM_TalentDict);

            g_modItemsInited = true;
            LOG_INFO("Mod SDK batch1 ready (ItemManager/ConfigManager/Attr full)");
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModItemsInit exception 0x%08X", GetExceptionCode());
            return false;
        }
    }

    // BattleLogicWorld.Instance -> ItemManager
    static Il2CppObject* GetItemManager()
    {
        __try
        {
            BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
            return world ? (Il2CppObject*)world->_ItemManager : nullptr;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    // 主角 InstanceId（AgentManager._LeadingRoleId）
    static int64_t GetPlayerId()
    {
        __try
        {
            BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
            if (!world || !world->_AgentManager)
                return 0;
            return world->_AgentManager->_LeadingRoleId;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    // BaseSingleton<ConfigManager>.Instance
    static Il2CppObject* GetConfigManager()
    {
        return GetSingletonInstance(g_klassConfigManager);
    }

    // 配置条目
    static Config_Item_o* GetConfigItem(int32_t configId)
    {
        __try
        {
            Il2CppObject* cm = GetConfigManager();
            if (!cm || !g_miConfigGetItem)
                return nullptr;
            return (Config_Item_o*)InvokeIntArg(g_miConfigGetItem, cm, configId);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    // 当前游戏时间（startTime 用；CountDownTimer=Total-Remain，CountUpTimer=CurrentTime，兜底 GetTime()）
    static int32_t GetCurrentGameTime()
    {
        __try
        {
            BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
            if (!world || !world->_GameTimeManager)
                return 0;
            GameTimeManager_o* gtm = world->_GameTimeManager;
            if (!gtm->_Timer)
                return 0;
            void* timer = gtm->_Timer;
            Il2CppClass* k = g_IL2CPP->object_get_class((Il2CppObject*)timer);
            if (k == g_klassCountDownTimer)
            {
                CountDownTimer_o* cdt = (CountDownTimer_o*)timer;
                return (int32_t)(cdt->_TotalTime - cdt->_RemainTime);
            }
            if (k == g_klassCountUpTimer)
                return (int32_t)((CountUpTimer_o*)timer)->CurrentTime;
            MethodInfo* mi = FindMethodInHierarchy(k, "GetTime", 0);
            if (mi)
                return UnboxInt32((Il2CppObject*)InvokeNoArg(mi, timer), 0);
            return 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    // 属性缩放（GameKey.Attr_ScalingRatio，默认 1000；mod AttrScale 同款）
    static int32_t AttrScale()
    {
        float r = GetStaticFloat(g_klassGameKey, "Attr_ScalingRatio", 1000.0f);
        if (!(r > 0.0f) || r > 1e9f)
            return 1000;
        return (int32_t)(r + 0.5f);
    }

    static int32_t SaturatingInt(int64_t v)
    {
        if (v > INT32_MAX)
            return INT32_MAX;
        if (v < INT32_MIN)
            return INT32_MIN;
        return (int32_t)v;
    }

    // AttrName 映射：Max(101-105) -> 核心(1-5)
    static int32_t MaxToCore(int32_t maxName)
    {
        int32_t d = maxName - 101;
        return (d >= 0 && d <= 4) ? (d + 1) : 0;
    }
    static int32_t CoreToMax(int32_t coreName)
    {
        int32_t d = coreName - 1;
        return (d >= 0 && d <= 4) ? (101 + d) : 0;
    }

    // 从 AttributeComponent.AttrDict 拿指定 Attr 条目
    static Attr_o* GetAttrEntry(int32_t attrName)
    {
        __try
        {
            AttributeComponent_o* comp = GetAttributeComponent();
            if (!comp || !comp->AttrDict)
                return nullptr;
            return DictFindAttr(comp->AttrDict, attrName, g_klassAttr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    // mod SetCurrentAttrDirect：写 BaseValue（含 Max/Min 维护）+ SyncAttr2UI
    static bool SetCurrentAttrDirect(int32_t name, int32_t value)
    {
        __try
        {
            AttributeComponent_o* comp = GetAttributeComponent();
            Attr_o* attr = GetAttrEntry(name);
            if (!comp || !attr)
                return false;
            int32_t scale = AttrScale();
            int64_t num2 = (int64_t)value * scale;
            int32_t maxName = CoreToMax(name);
            if (maxName > 0)
            {
                int32_t maxAttr = SLSDK_GetAttrMax(maxName);
                if (maxAttr >= 0)
                {
                    int64_t num3 = (int64_t)maxAttr * scale;
                    if (num2 > num3)
                        num2 = num3;
                    int32_t n4 = SaturatingInt(num3);
                    if (attr->Max < n4)
                        attr->Max = n4;
                }
            }
            else if (attr->Max < num2)
            {
                attr->Max = SaturatingInt(num2);
            }
            if (attr->Min > num2)
                attr->Min = SaturatingInt(num2);
            attr->BaseValue = SaturatingInt(num2 - attr->StrengtheningValue);
            if (g_miSyncAttr2UI)
                InvokeIntArg(g_miSyncAttr2UI, comp, name);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // mod SetMaxAttrDirect：写上限条目 + 联动核心属性
    static bool SetMaxAttrDirect(int32_t maxName, int32_t value)
    {
        __try
        {
            AttributeComponent_o* comp = GetAttributeComponent();
            Attr_o* attr = GetAttrEntry(maxName);
            if (!comp || !attr)
                return false;
            int32_t coreName = MaxToCore(maxName);
            Attr_o* attr2 = coreName > 0 ? GetAttrEntry(coreName) : nullptr;
            float coreVal = -1.0f;
            if (attr2 && g_miGetTotalValueFloat)
            {
                void* p[1] = { &coreName };
                coreVal = UnboxFloat((Il2CppObject*)InvokeRet(g_miGetTotalValueFloat, comp, p), -1.0f);
            }
            attr->BaseValue = SaturatingInt((int64_t)value - attr->StrengtheningValue);
            attr->Max = INT32_MAX;
            if (attr->Min > value)
                attr->Min = value;
            if (attr2)
            {
                int32_t num2 = SaturatingInt((int64_t)value * AttrScale());
                attr2->Max = INT32_MAX;
                if (attr2->Min > num2)
                    attr2->Min = num2;
                if (coreVal > (float)value)
                    SetCurrentAttrDirect(coreName, value);
                else if (g_miSyncAttr2UI)
                    InvokeIntArg(g_miSyncAttr2UI, comp, coreName);
            }
            if (g_miSyncAttr2UI)
                InvokeIntArg(g_miSyncAttr2UI, comp, maxName);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // 移速基线缓存（mod EnsureMoveSpeedBaseline 同款）
    void* g_moveSpeedOwner = nullptr;
    float g_originalMoveSpeedBase = -1.0f;
    float g_moveSpeedMultiplier = 1.0f;

    static bool IsValidSpeed(float v)
    {
        return v > 0.0f && v < 1e9f;
    }

    static float GetConfiguredMoveSpeed()
    {
        __try
        {
            LeadingRole_o* leading = GetLeadingRole();
            if (!leading || leading->AgentConfigId <= 0)
                return -1.0f;
            Il2CppObject* cm = GetConfigManager();
            if (!cm || !g_miConfigGetPlayer || !g_miConfigGetPlayAttr)
                return -1.0f;
            Config_Player_o* cfgPlayer = (Config_Player_o*)InvokeIntArg(g_miConfigGetPlayer, cm, leading->AgentConfigId);
            if (!cfgPlayer || cfgPlayer->LinkAttr <= 0)
                return -1.0f;
            Config_PlayAttribute_o* cfgAttr = (Config_PlayAttribute_o*)InvokeIntArg(g_miConfigGetPlayAttr, cm, cfgPlayer->LinkAttr);
            return cfgAttr ? cfgAttr->MoveSpeed : -1.0f;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return -1.0f;
        }
    }

    static bool EnsureMoveSpeedBaseline()
    {
        AttributeComponent_o* comp = GetAttributeComponent();
        if (!comp)
            return false;
        if (g_moveSpeedOwner == comp && IsValidSpeed(g_originalMoveSpeedBase))
            return true;
        float num = GetConfiguredMoveSpeed();
        if (!IsValidSpeed(num) && g_miGetBaseValueFloat)
        {
            int32_t n401 = 401;
            void* p[1] = { &n401 };
            num = UnboxFloat((Il2CppObject*)InvokeRet(g_miGetBaseValueFloat, comp, p), -1.0f);
        }
        if (!IsValidSpeed(num))
            return false;
        g_moveSpeedOwner = comp;
        g_originalMoveSpeedBase = num;
        g_moveSpeedMultiplier = 1.0f;
        return true;
    }
}



// ---------- 物品 / 背包 ----------
bool SLSDK_AddItem(int32_t configId, int32_t count, int32_t* addedOut)
{
    if (addedOut)
        *addedOut = 0;
    if (count <= 0 || !ModItemsInit())
        return false;
    __try
    {
        Il2CppObject* items = GetItemManager();
        int64_t playerId = GetPlayerId();
        Config_Item_o* cfg = GetConfigItem(configId);
        if (!items || playerId == 0 || !cfg)
            return false;
        int32_t itemW = IntListGet(cfg->Size, 0, 1);
        int32_t itemH = IntListGet(cfg->Size, 1, 1);
        if (itemW < 1)
            itemW = 1;
        if (itemH < 1)
            itemH = 1;
        int32_t startTime = GetCurrentGameTime();
        int64_t life = (cfg->Life > 0) ? (int64_t)cfg->Life * 24 : 9999999;
        int32_t shelfLife = (life > INT32_MAX) ? INT32_MAX : (int32_t)life;
        int32_t stackLimit = (cfg->StackLimit > 0) ? cfg->StackLimit : 9999;
        int32_t remaining = count;
        int32_t added = 0;
        while (remaining > 0)
        {
            Vector2Int_o itemSize = { itemW, itemH };
            Vector2Int_o zero = { 0, 0 };
            Vector2Int_o pos = { 0, 0 };
            void* p1[4] = { &playerId, &itemSize, &zero, &pos };
            bool hasPos = UnboxBool((Il2CppObject*)InvokeRet(g_miItemTryResolveBagPos, items, p1), false);
            if (!hasPos)
                break; // 无空位（mod 有扩容分支，C++ 先不做扩容）
            int32_t batch = (remaining < stackLimit) ? remaining : stackLimit;
            uint8_t bFalse = 0;
            void* p2[8] = { &playerId, &configId, &batch, &startTime, &shelfLife, &pos, &bFalse, &bFalse };
            Il2CppException* exc = nullptr;
            Il2CppObject* newItem = g_IL2CPP->runtime_invoke(g_miItemAddItem, items, p2, &exc);
            if (exc || !newItem)
                break;
            added += batch;
            remaining -= batch;
        }
        if (addedOut)
            *addedOut = added;
        return added > 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

int32_t SLSDK_ListBackpackItems(SLItemView* outItems, int32_t maxItems)
{
    if (!ModItemsInit())
        return 0;
    __try
    {
        Il2CppObject* items = GetItemManager();
        int64_t playerId = GetPlayerId();
        if (!items || playerId == 0 || !g_miItemGetItemDataList)
            return 0;
        void* p[1] = { &playerId };
        Il2CppObject* list = InvokeRet(g_miItemGetItemDataList, items, p);
        if (!list)
            return 0;
        size_t n = ListGetCount(list);
        if (n > 100000)
            n = 100000;
        int32_t written = 0;
        for (size_t i = 0; i < n && (outItems == nullptr || written < maxItems); i++)
        {
            ItemData_o* item = (ItemData_o*)ListGetItem(list, i);
            if (!item)
                continue;
            if (outItems)
            {
                outItems[written].InstanceId = item->InstanceId;
                
                outItems[written].ConfigId = item->ItemConfigId;
                outItems[written].Count = item->ItemCount;
                outItems[written].TimeScale = item->TimeScale;
            }
            written++;
        }
        return written;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

bool SLSDK_SetBackpackItemCount(int64_t instanceId, int32_t newCount)
{
    if (!ModItemsInit())
        return false;
    __try
    {
        Il2CppObject* items = GetItemManager();
        if (!items || !g_miItemGetItemData)
            return false;
        void* p1[1] = { &instanceId };
        ItemData_o* itemData = (ItemData_o*)InvokeRet(g_miItemGetItemData, items, p1);
        if (!itemData)
            return false;
        Config_Item_o* cfg = GetConfigItem(itemData->ItemConfigId);
        if (!cfg)
            return false;
        int32_t stackLimit = (cfg->StackLimit > 0) ? cfg->StackLimit : 9999;
        int32_t num2 = (newCount < stackLimit) ? newCount : stackLimit;
        void* p2[2] = { &itemData, &num2 };
        if (!InvokeOk(g_miItemSetItemCount, items, p2))
            return false;
        int32_t num3 = newCount - num2;
        if (num3 > 0)
        {
            int32_t added = 0;
            if (!SLSDK_AddItem(itemData->ItemConfigId, num3, &added) || added < num3)
                return false;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_DuplicateBackpackItem(int64_t instanceId)
{
    if (!ModItemsInit())
        return false;
    __try
    {
        Il2CppObject* items = GetItemManager();
        if (!items || !g_miItemGetItemData)
            return false;
        void* p[1] = { &instanceId };
        ItemData_o* itemData = (ItemData_o*)InvokeRet(g_miItemGetItemData, items, p);
        if (!itemData)
            return false;
        int32_t itemCount = itemData->ItemCount;
        if (itemCount <= 0)
            return false;
        int32_t added = 0;
        if (!SLSDK_AddItem(itemData->ItemConfigId, itemCount, &added))
            return false;
        return added == itemCount;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_RemoveBackpackItem(int64_t instanceId)
{
    if (!ModItemsInit())
        return false;
    __try
    {
        Il2CppObject* items = GetItemManager();
        if (!items || !g_miItemGetItemData)
            return false;
        void* p1[1] = { &instanceId };
        if (!InvokeRet(g_miItemGetItemData, items, p1))
            return false;
        void* p2[1] = { &instanceId };
        return InvokeOk(g_miItemForceRemoveItem, items, p2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}


// ---------- 物品目录（mod ItemCatalog 同款） ----------
namespace
{
    std::vector<SLItemInfo> g_itemCatalog;
    bool g_catalogBuilt = false;
}
const char* SLSDK_GetItemName(int32_t configId)
{
	if (configId <= 0)
		return {};
	if (!g_catalogBuilt && g_itemCatalog.empty())
		SLSDK_RefreshItemCatalog();
	for (const SLItemInfo& info : g_itemCatalog)
	{
		if (info.Id == configId)
			return info.Name;
	}
	return {};
}   
int32_t SLSDK_RefreshItemCatalog()
{
    if (!ModItemsInit())
        return -1;
    __try
    {
        Il2CppObject* cm = GetConfigManager();
        if (!cm)
            return -1;
        void* dict = *(void**)((uint8_t*)cm + OFF_CM_ItemDict); // ConfigManager._Config_Item_Dict
        if (!dict)
            return -1;
        Dictionary_o* d = (Dictionary_o*)dict;
        if (!d->_entries || d->_count <= 0)
            return -1;
        g_itemCatalog.clear();
        g_itemCatalog.reserve((size_t)d->_count);
        int32_t n = d->_count;
        if (n > 100000)
            n = 100000;
        uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA; // Entry 数组数据区
        for (int32_t i = 0; i < n; i++)
        {
            uint8_t* entry = entries + (size_t)i * 24; // hashCode(4) next(4) key(8) value(8)
            void* value = *(void**)(entry + 16);
            if (!value)
                continue;
            Config_Item_o* cfg = (Config_Item_o*)value;
            SLItemInfo info = {};
            info.Id = cfg->ID;
            info.Category = cfg->Category;
            info.SubCategory = cfg->SubCategory;
            info.Price = cfg->price;
            info.Weight = cfg->weight;
            char tmp[512];
            ILStringToUtf8(cfg->ItemName_Local, tmp, sizeof(tmp));
            if (!tmp[0])
                ILStringToUtf8(cfg->ItemName, tmp, sizeof(tmp));
            if (!tmp[0])
                snprintf(tmp, sizeof(tmp), "#%d", cfg->ID);
            // strncpy 触发 C4996，用 memcpy 手动截断
            size_t ncopy = strlen(tmp);
            if (ncopy >= sizeof(info.Name))
                ncopy = sizeof(info.Name) - 1;
            memcpy(info.Name, tmp, ncopy);
            info.Name[ncopy] = 0;
            g_itemCatalog.push_back(info);
        }
        g_catalogBuilt = true;
        return (int32_t)g_itemCatalog.size();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

int32_t SLSDK_GetItemCatalog(SLItemInfo* outItems, int32_t maxItems)
{
    if (!g_catalogBuilt && g_itemCatalog.empty())
    {
        if (SLSDK_RefreshItemCatalog() < 0)
            return 0;
    }
    size_t n = g_itemCatalog.size();
    if (outItems && maxItems > 0)
    {
        size_t c = (n < (size_t)maxItems) ? n : (size_t)maxItems;
        memcpy(outItems, g_itemCatalog.data(), c * sizeof(SLItemInfo));
        return (int32_t)c;
    }
    return (int32_t)n;
}

// ---------- 属性（mod 完整版） ----------
int32_t SLSDK_GetAttr(int32_t attrName)
{
    if (!ModItemsInit())
        return -1;
    __try
    {
        AttributeComponent_o* comp = GetAttributeComponent();
        if (!comp || !g_miGetTotalValueFloat)
            return -1;
        void* p[1] = { &attrName };
        float v = UnboxFloat((Il2CppObject*)InvokeRet(g_miGetTotalValueFloat, comp, p), -1.0f);
        if (v < 0.0f)
            return -1;
        return (int32_t)(v + (v >= 0.0f ? 0.5f : -0.5f)); // Mathf.RoundToInt 近似
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

int32_t SLSDK_GetAttrMax(int32_t attrName)
{
    if (!ModItemsInit())
        return -1;
    __try
    {
        AttributeComponent_o* comp = GetAttributeComponent();
        if (!comp || !g_miGetTotalValueInt)
            return -1;
        void* p[1] = { &attrName };
        return UnboxInt32((Il2CppObject*)InvokeRet(g_miGetTotalValueInt, comp, p), -1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

bool SLSDK_SetAttr(int32_t attrName, int32_t value)
{
    if (!ModItemsInit())
        return false;
    __try
    {
        int32_t d = attrName - 101;
        bool isMax = (d >= 0 && d <= 4);
        if (isMax)
            return SetMaxAttrDirect(attrName, value);
        int32_t maxName = CoreToMax(attrName);
        if (maxName > 0)
        {
            int32_t maxAttr = SLSDK_GetAttrMax(maxName);
            if (maxAttr >= 0 && value > maxAttr)
                return false; // mod：当前值不能超过上限
        }
        return SetCurrentAttrDirect(attrName, value);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_SetAttrMax(int32_t attrName, int32_t value)
{
    if (!ModItemsInit())
        return false;
    return SetMaxAttrDirect(attrName, value);
}

void SLSDK_ApplyAttrLocks(bool hp, bool stamina, bool satiety, bool morale)
{
    if (!hp && !stamina && !satiety && !morale)
        return;
    if (!ModItemsInit())
        return;
    __try
    {
        // mod TopUp：curName/maxName 映射，锁值=-1 时取 maxAttr
        struct LockPair { int32_t cur; int32_t max; };
        LockPair pairs[4] = {
            { 5, 105 }, // hp -> Vitality/MaxVitality
            { 3, 103 }, // stamina
            { 1, 101 }, // satiety
            { 2, 102 }, // morale
        };
        bool flags[4] = { hp, stamina, satiety, morale };
        for (int i = 0; i < 4; i++)
        {
            if (!flags[i])
                continue;
            int32_t curName = pairs[i].cur;
            int32_t maxName = pairs[i].max;
            int32_t maxAttr = SLSDK_GetAttrMax(maxName);
            int32_t num = (maxAttr >= 0) ? maxAttr : -1;
            if (num < 0)
                continue;
            AttributeComponent_o* comp = GetAttributeComponent();
            if (!comp || !g_miGetTotalValueFloat)
                continue;
            void* p[1] = { &curName };
            float curVal = UnboxFloat((Il2CppObject*)InvokeRet(g_miGetTotalValueFloat, comp, p), -1.0f);
            if (curVal < 0.0f || (curVal - (float)num) > 0.001f || ((float)num - curVal) > 0.001f)
                SetCurrentAttrDirect(curName, num);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

bool SLSDK_GetMoveSpeed(float* current, float* original, float* multiplier)
{
    if (current)
        *current = -1.0f;
    if (original)
        *original = -1.0f;
    if (multiplier)
        *multiplier = 1.0f;
    if (!ModItemsInit())
        return false;
    __try
    {
        AttributeComponent_o* comp = GetAttributeComponent();
        if (!comp || !EnsureMoveSpeedBaseline() || !g_miGetTotalValueFloat)
            return false;
        int32_t n401 = 401;
        void* p[1] = { &n401 };
        float cur = UnboxFloat((Il2CppObject*)InvokeRet(g_miGetTotalValueFloat, comp, p), -1.0f);
        if (!IsValidSpeed(cur))
            return false;
        if (current)
            *current = cur;
        if (original)
            *original = g_originalMoveSpeedBase;
        if (multiplier)
            *multiplier = g_moveSpeedMultiplier;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_SetMoveSpeedMultiplier(float multiplier)
{
    if (!ModItemsInit())
        return false;
    __try
    {
        AttributeComponent_o* comp = GetAttributeComponent();
        if (!comp || !EnsureMoveSpeedBaseline() || !g_miSetBaseValueFloat)
            return false;
        if (!(multiplier >= 0.5f) || !(multiplier <= 5.0f))
            return false;
        int32_t n401 = 401;
        float target = g_originalMoveSpeedBase * multiplier;
        uint8_t bTrue = 1;
        void* p[3] = { &n401, &target, &bTrue };
        if (!InvokeOk(g_miSetBaseValueFloat, comp, p))
            return false;
        if (g_miSyncAttr2UI)
            InvokeIntArg(g_miSyncAttr2UI, comp, n401);
        g_moveSpeedMultiplier = multiplier;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_ResetMoveSpeed()
{
    if (!ModItemsInit())
        return false;
    __try
    {
        AttributeComponent_o* comp = GetAttributeComponent();
        if (!comp || !EnsureMoveSpeedBaseline() || !g_miSetBaseValueFloat)
            return false;
        int32_t n401 = 401;
        float base = g_originalMoveSpeedBase;
        uint8_t bTrue = 1;
        void* p[3] = { &n401, &base, &bTrue };
        if (!InvokeOk(g_miSetBaseValueFloat, comp, p))
            return false;
        if (g_miSyncAttr2UI)
            InvokeIntArg(g_miSyncAttr2UI, comp, n401);
        g_moveSpeedMultiplier = 1.0f;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ============================================================
// 批次2：Buff / 生存规划 / 熟练度 / 关系 / 图鉴成就 / 暴露生存点移动热键 / 时间
// 全部通过 GetLeadingRoleComponent 拿组件 + 按名解析方法，失败降级不崩
// ============================================================

// ---------- 批次2 结构体（偏移来自 SDK dump） ----------
struct BuffComponent_o : BaseComponent_o
{
    void* buffIdMap;       // +0x40 Dictionary<int, List<BuffArgs>>
    void* buffArgsMap;     // +0x48 Dictionary<long, BuffArgs>
    void* effectiveList;   // +0x50 List<BuffArgs>
    void* talentRatioCache; // +0x58
    void* buffIds;         // +0x60 List<int>
};

struct BuffArgs_o : Il2CppObject
{
    uint8_t _pad[0x20];      // 基类字段（+0x10 ~ +0x2F，dump 显示 config 从 +0x30 起）
    void* config;            // +0x30 Config_Buff
    int64_t releaserId;      // +0x38
    int64_t EffectBtArgsID;  // +0x40
    int32_t BuffCount;       // +0x48
    int64_t buffShowInstanceId; // +0x50
    int32_t TimeEndTime;     // +0x58
};

struct Config_Buff_o : Il2CppObject
{
    int32_t ID;              // +0x10
    void* Name;              // +0x18
    void* Name_Local;        // +0x20
    bool IsGood;             // +0x28
    int32_t BuffDisplayType; // +0x2C
    int32_t BuffType;        // +0x30
    bool IsHide;             // +0x34
    int32_t Mask;            // +0x38
    void* Des;               // +0x40
    void* Des_Local;         // +0x48
    float BuffDuring;        // +0x80（跳过中间列表字段）
};

struct Config_Talent_o : Il2CppObject
{
    int32_t ID;              // +0x10
    int32_t TalentID;        // +0x14
    int32_t Lv;              // +0x18
    int32_t Available;       // +0x1C
    int32_t DemoTwoMode;     // +0x20
    void* Name;              // +0x28
    void* Name_Local;        // +0x30
    void* Dec;               // +0x38
    void* Dec_Local;         // +0x40
    int32_t Type;            // +0x48
    bool IsReget;            // +0x4C
    int32_t ShowType;        // +0x50
    void* Icon;              // +0x58
    void* WebIcon;           // +0x60
    int32_t BuffID;          // +0x68
    int32_t BuffRefresh;     // +0x6C
};

struct Config_DailyRandom_o : Il2CppObject
{
    int32_t Day;             // +0x10
    void* RandomPlanID;      // +0x18 List<int>
    void* RandomAmount;      // +0x20 List<int>
    void* SpecifiedPlanID;   // +0x28 List<int>
};

struct Config_RandomGroup_o : Il2CppObject
{
    int32_t ID;              // +0x10
    void* RandomGroupName;   // +0x18
    void* RandomGroupName_Local; // +0x20
    int32_t GroupType;       // +0x28
    void* IdList;            // +0x30 List<int>
};

struct SurvivalPlanningComponent_o : BaseComponent_o
{
    void* SaveCache;         // +0x40 List<int>
};

struct ExploreManager_o : BaseEntity_o
{
    bool IsLocked;           // +0x30
    float CurExposure;       // +0x34
    int32_t _maxExposureBase;// +0x38
    float TimeExposure;      // +0x3C
    float ExposureAlarm;     // +0x40
    float MoveExposure;      // +0x44
    bool IsRunning;          // +0x48
};

struct SurvivalResultsManager_o : BaseEntity_o
{
    int32_t SurvivalPoint;   // +0x30
    int32_t TotalPoints;     // +0x34
};

struct ProficiencySystemSnapshot_o : Il2CppObject
{
    int32_t SystemId;        // +0x10
    void* SystemName;        // +0x18
    int32_t CurrentLevel;    // +0x20
    int32_t CurrentExp;      // +0x24
    int32_t NextLevelExp;    // +0x28
    int32_t PrevLevelExp;    // +0x2C
};

struct BattleShowWorld_o : Il2CppObject
{
    int64_t InstanceId;          // +0x10（BaseSingleton_Fields）
    bool IsUpdate;               // +0x18
    bool IsFloorExpansionEnabled; // +0x19
    void* _MapNodeSearch;        // +0x20
    void* _AgentModelManager;    // +0x28
    void* _HitTextManager;       // +0x30
    void* _CameraManager;        // +0x38
    void* _LightManager;         // +0x40
    void* _BtShowManager;        // +0x48
    void* _ColliderMapManager;   // +0x50
    void* _HotKeyManager;        // +0x58
};

struct CameraManager_o : Il2CppObject
{
    uint8_t _pad[0x176];         // 前面全是相机字段，不逐个定义
    bool isKeyboardMoveBlocked;  // +0x176
};

namespace
{
    // ---------- 批次2 类/方法缓存 ----------
    Il2CppClass* g_klassBuffComponent = nullptr;
    Il2CppClass* g_klassBuffArgs = nullptr;
    Il2CppClass* g_klassConfigBuff = nullptr;
    Il2CppClass* g_klassConfigTalent = nullptr;
    Il2CppClass* g_klassConfigDailyRandom = nullptr;
    Il2CppClass* g_klassConfigRandomGroup = nullptr;
    Il2CppClass* g_klassSurvivalPlanning = nullptr;
    Il2CppClass* g_klassNeighborRescue = nullptr;
    Il2CppClass* g_klassCodexManager = nullptr;
    Il2CppClass* g_klassAchievementManager = nullptr;
    Il2CppClass* g_klassExploreManager = nullptr;
    Il2CppClass* g_klassSurvivalResults = nullptr;
    Il2CppClass* g_klassProficiencyManager = nullptr;
    Il2CppClass* g_klassProficiencySnapshot = nullptr;
    Il2CppClass* g_klassBattleShowWorld = nullptr;
    Il2CppClass* g_klassCameraManager = nullptr;
    Il2CppClass* g_klassHotKeyManager = nullptr;
    Il2CppClass* g_klassWebGm = nullptr;
    Il2CppClass* g_klassToolset = nullptr;

    MethodInfo* g_miConfigGetBuff = nullptr;        // Get_Config_Buff(int)
    MethodInfo* g_miConfigGetTalent = nullptr;      // Get_Config_Talent(int)
    MethodInfo* g_miConfigGetTalent2 = nullptr;     // GetTalentConfig(int,int)
    MethodInfo* g_miConfigGetRandomGroup = nullptr; // Get_Config_RandomGroup(int)
    MethodInfo* g_miBuffGetBuffConfigIds = nullptr; // GetBuffConfigIds()
    MethodInfo* g_miBuffGetEditorBuffMap = nullptr; // GetEditorBuffMap()
    MethodInfo* g_miBuffGetEffectiveList = nullptr; // GetEffectiveBuffList()
    MethodInfo* g_miBuffAddBuff = nullptr;          // AddBuff(long,int)
    MethodInfo* g_miBuffRemoveByConfig = nullptr;   // RemoveBuff(int)
    MethodInfo* g_miBuffRemoveByInstance = nullptr; // RemoveBuff(long)
    MethodInfo* g_miBuffRequestRefresh = nullptr;   // RequestLeadingRoleBuffRefresh()
    MethodInfo* g_miPlanHasActivated = nullptr;     // HasActivated(int)
    MethodInfo* g_miPlanAddBuff = nullptr;          // AddBuff(BuffComponent,int)
    MethodInfo* g_miPlanRemoveBuff = nullptr;       // RemoveBuff(BuffComponent,int)
    MethodInfo* g_miPlanNotifyUpdate = nullptr;     // NotifySurvivalPlanningUpdate()
    MethodInfo* g_miNeighborGetAffinity = nullptr;  // GmGetAffinity()
    MethodInfo* g_miNeighborGetTier = nullptr;      // GetAffinityDisplayTier()
    MethodInfo* g_miNeighborIsUnlocked = nullptr;   // GmIsUnlocked()
    MethodInfo* g_miNeighborAddAffinity = nullptr;  // AddAffinity(int,long)
    MethodInfo* g_miCodexUnlockAll = nullptr;       // GmUnlockAll()
    MethodInfo* g_miAchieveUnlockAll = nullptr;     // GmUnlockAll()
    MethodInfo* g_miExploreNotifyUI = nullptr;      // NotifyUI()
    MethodInfo* g_miExploreGetMax = nullptr;        // get_MaxExposure()
    MethodInfo* g_miSurvResultGetPoint = nullptr;   // GetSurvivalPoint()
    MethodInfo* g_miSurvResultAddPoint = nullptr;   // AddSurvivalPoint(int)
    MethodInfo* g_miProfGetSnapshot = nullptr;      // GetSnapshot()
    MethodInfo* g_miProfIsMaxLevel = nullptr;       // IsMaxLevel(ProficiencyType)
    MethodInfo* g_miProfGetLevel = nullptr;         // GetLevel(ProficiencyType)
    MethodInfo* g_miProfAddExp = nullptr;           // AddExp(ProficiencyType,string,int,int,int,ProficiencyExpSource) 6参
    MethodInfo* g_miProfAddLevel = nullptr;         // AddLevel(ProficiencyType,int,bool) 3参
    MethodInfo* g_miHotKeyDisable = nullptr;        // OnDisableResponse(bool)
    MethodInfo* g_miTimeGetDay = nullptr;           // GetDay()
    MethodInfo* g_miTimeGetHour = nullptr;          // GetHour()
    MethodInfo* g_miTimeGetTotalSeconds = nullptr;  // GetTotalSeconds()
    MethodInfo* g_miTimeGetRemainHour = nullptr;    // GetRemainHourFloat()
    MethodInfo* g_miTimeAddExtraCountDown = nullptr;// AddExtraCountDownTime(int)
    MethodInfo* g_miTimeNotifyScaleSync = nullptr;  // NotifyCountDownTimerScaleSync()
    MethodInfo* g_miTimeSetClockFrozen = nullptr;   // set_IsClockFrozen(bool)（mod 走 setter，有副作用）
    MethodInfo* g_miCdtGetRemainHour = nullptr;     // CountDownTimer.GetRemainHourFloat()
    MethodInfo* g_miToolsetGameTimeFloat = nullptr; // Toolset.GetGameTime_Float(int) 静态
    bool g_modBatch2Inited = false;

    // 冻结/防暴露状态（mod FrozenOverride / NoExploreExposure 对应）
    bool g_frozenOverride = false;
    bool g_noExploreExposure = false;

    // MethodInfo 内部布局（标准 IL2CPP 64 位，Unity 2021+）：
    //   +0x00 methodPointer, +0x08 invoker, +0x10 name, +0x18 klass,
    //   +0x20 return_type, +0x28 parameters(ParameterInfo*), +0x30 metadata,
    //   +0x40 token(u32), +0x44 flags(u16), +0x46 slot(u16), +0x48 parameters_count(u8)
    // ParameterInfo：+0x00 name, +0x08 token, +0x10 parameter_type(Il2CppType*)
    // Il2CppType.type 枚举在 +0x0A（8 位）
    static int32_t MethodParamCount(const MethodInfo* m)
    {
        if (!m || !g_IL2CPP || !g_IL2CPP->method_get_param_count)
            return -1;
        return (int32_t)g_IL2CPP->method_get_param_count(m);
    }

    // 按序号取「同名 + 同参数个数」的方法（nth 从 0 开始）
    // 用途：dump.cs 确认 RemoveBuff 重载顺序 = 0:long 版, 1:int 版
    static MethodInfo* FindNthMethod(Il2CppClass* klass, const char* name, int argc, int nth)
    {
        if (!klass || !g_IL2CPP)
            return nullptr;
        int found = 0;
        __try
        {
            for (Il2CppClass* k = klass; k; k = g_IL2CPP->class_get_parent(k))
            {
                void* iter = nullptr;
                const MethodInfo* m;
                while ((m = g_IL2CPP->class_get_methods(k, &iter)))
                {
                    const char* n = g_IL2CPP->method_get_name ? g_IL2CPP->method_get_name(m) : nullptr;
                    if (!n || strcmp(n, name) != 0)
                        continue;
                    if (MethodParamCount(m) != argc)
                        continue;
                    if (found == nth)
                        return (MethodInfo*)m;
                    found++;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return nullptr;
    }

    // 生存规划操作失败原因诊断（AddSurvivalPlan/RemoveSurvivalPlan 失败时填充）
    static char g_planError[256] = {};
    static const char* g_planStep = "";

    static bool ModBatch2Init()
    {
        if (g_modBatch2Inited)
            return true;
        // 先确保批次1 类已解析（ConfigManager/GameTimeManager/CountDownTimer 等）
        if (!ModItemsInit())
            return false;
        if (!SLSDK_Ready() || !g_IL2CPP)
            return false;
        bool segOk = true;
        // ===== 段1：类解析（失败则整体不可用） =====
        __try
        {
            if (!g_hotUpdateImage)
                return false;
            g_klassBuffComponent = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "BuffComponent");
            g_klassBuffArgs = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "BuffArgs");
            g_klassConfigBuff = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_Buff");
            g_klassConfigTalent = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_Talent");
            g_klassConfigDailyRandom = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_DailyRandom");
            g_klassConfigRandomGroup = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_RandomGroup");
            g_klassSurvivalPlanning = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "SurvivalPlanningComponent");
            g_klassNeighborRescue = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "NeighborRescueManager");
            g_klassCodexManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "CodexManager");
            g_klassAchievementManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "AchievementManager");
            g_klassExploreManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "ExploreManager");
            g_klassSurvivalResults = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "SurvivalResultsManager");
            g_klassProficiencyManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "ProficiencyManager");
            g_klassProficiencySnapshot = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "ProficiencySystemSnapshot");
            g_klassBattleShowWorld = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Show", "BattleShowWorld");
            g_klassCameraManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Show", "CameraManager");
            g_klassHotKeyManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Show", "HotKeyManager");
            g_klassWebGm = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "WebGm");
            g_klassToolset = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Toolset");
            // 全类检查：哪个类为 null 一目了然
            struct { Il2CppClass* k; const char* n; } chk[] = {
                { g_klassBuffComponent, "BuffComponent" },
                { g_klassBuffArgs, "BuffArgs" },
                { g_klassConfigBuff, "Config_Buff" },
                { g_klassConfigTalent, "Config_Talent" },
                { g_klassConfigDailyRandom, "Config_DailyRandom" },
                { g_klassConfigRandomGroup, "Config_RandomGroup" },
                { g_klassSurvivalPlanning, "SurvivalPlanningComponent" },
                { g_klassNeighborRescue, "NeighborRescueManager" },
                { g_klassCodexManager, "CodexManager" },
                { g_klassAchievementManager, "AchievementManager" },
                { g_klassExploreManager, "ExploreManager" },
                { g_klassSurvivalResults, "SurvivalResultsManager" },
                { g_klassProficiencyManager, "ProficiencyManager" },
                { g_klassProficiencySnapshot, "ProficiencySystemSnapshot" },
                { g_klassBattleShowWorld, "BattleShowWorld" },
                { g_klassCameraManager, "CameraManager" },
                { g_klassHotKeyManager, "HotKeyManager" },
                { g_klassWebGm, "WebGm" },
                { g_klassToolset, "Toolset" },
                { g_klassGameTimeManager, "GameTimeManager" },
                { g_klassCountDownTimer, "CountDownTimer" },
                { g_klassConfigManager, "ConfigManager" },
            };
            bool allOk = true;
            for (size_t ci = 0; ci < sizeof(chk) / sizeof(chk[0]); ci++)
            {
                if (!chk[ci].k)
                {
                    LOG_ERROR("ModBatch2 class NULL: %s", chk[ci].n);
                    allOk = false;
                }
            }
            if (!allOk)
                return false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModBatch2 seg1 class resolve exception 0x%08X", GetExceptionCode());
            return false;
        }
        // ===== 段2：普通方法解析（按名+参数个数） =====
        __try
        {
            if (g_klassConfigManager)
            {
                g_miConfigGetBuff = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "Get_Config_Buff", 1);
                g_miConfigGetTalent = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "Get_Config_Talent", 1);
                g_miConfigGetTalent2 = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "GetTalentConfig", 2);
                g_miConfigGetRandomGroup = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "Get_Config_RandomGroup", 1);
            }
            if (g_klassBuffComponent)
            {
                g_miBuffGetBuffConfigIds = g_IL2CPP->class_get_method_from_name(g_klassBuffComponent, "GetBuffConfigIds", 0);
                g_miBuffGetEditorBuffMap = g_IL2CPP->class_get_method_from_name(g_klassBuffComponent, "GetEditorBuffMap", 0);
                g_miBuffGetEffectiveList = g_IL2CPP->class_get_method_from_name(g_klassBuffComponent, "GetEffectiveBuffList", 0);
                g_miBuffAddBuff = g_IL2CPP->class_get_method_from_name(g_klassBuffComponent, "AddBuff", 2);
                g_miBuffRequestRefresh = g_IL2CPP->class_get_method_from_name(g_klassBuffComponent, "RequestLeadingRoleBuffRefresh", 0);
            }
            if (g_klassSurvivalPlanning)
            {
                g_miPlanHasActivated = g_IL2CPP->class_get_method_from_name(g_klassSurvivalPlanning, "HasActivated", 1);
                g_miPlanAddBuff = g_IL2CPP->class_get_method_from_name(g_klassSurvivalPlanning, "AddBuff", 2);
                g_miPlanRemoveBuff = g_IL2CPP->class_get_method_from_name(g_klassSurvivalPlanning, "RemoveBuff", 2);
                g_miPlanNotifyUpdate = g_IL2CPP->class_get_method_from_name(g_klassSurvivalPlanning, "NotifySurvivalPlanningUpdate", 0);
            }
            if (g_klassNeighborRescue)
            {
                g_miNeighborGetAffinity = g_IL2CPP->class_get_method_from_name(g_klassNeighborRescue, "GmGetAffinity", 0);
                g_miNeighborGetTier = g_IL2CPP->class_get_method_from_name(g_klassNeighborRescue, "GetAffinityDisplayTier", 0);
                g_miNeighborIsUnlocked = g_IL2CPP->class_get_method_from_name(g_klassNeighborRescue, "GmIsUnlocked", 0);
                g_miNeighborAddAffinity = g_IL2CPP->class_get_method_from_name(g_klassNeighborRescue, "AddAffinity", 2);
            }
            if (g_klassCodexManager)
                g_miCodexUnlockAll = g_IL2CPP->class_get_method_from_name(g_klassCodexManager, "GmUnlockAll", 0);
            if (g_klassAchievementManager)
                g_miAchieveUnlockAll = g_IL2CPP->class_get_method_from_name(g_klassAchievementManager, "GmUnlockAll", 0);
            if (g_klassExploreManager)
            {
                g_miExploreNotifyUI = g_IL2CPP->class_get_method_from_name(g_klassExploreManager, "NotifyUI", 0);
                g_miExploreGetMax = g_IL2CPP->class_get_method_from_name(g_klassExploreManager, "get_MaxExposure", 0);
            }
            if (g_klassSurvivalResults)
            {
                g_miSurvResultGetPoint = g_IL2CPP->class_get_method_from_name(g_klassSurvivalResults, "GetSurvivalPoint", 0);
                g_miSurvResultAddPoint = g_IL2CPP->class_get_method_from_name(g_klassSurvivalResults, "AddSurvivalPoint", 1);
            }
            if (g_klassProficiencyManager)
            {
                g_miProfGetSnapshot = g_IL2CPP->class_get_method_from_name(g_klassProficiencyManager, "GetSnapshot", 0);
                g_miProfIsMaxLevel = g_IL2CPP->class_get_method_from_name(g_klassProficiencyManager, "IsMaxLevel", 1);
                g_miProfGetLevel = g_IL2CPP->class_get_method_from_name(g_klassProficiencyManager, "GetLevel", 1);
                g_miProfAddExp = g_IL2CPP->class_get_method_from_name(g_klassProficiencyManager, "AddExp", 6);
                g_miProfAddLevel = g_IL2CPP->class_get_method_from_name(g_klassProficiencyManager, "AddLevel", 3);
            }
            if (g_klassHotKeyManager)
                g_miHotKeyDisable = g_IL2CPP->class_get_method_from_name(g_klassHotKeyManager, "OnDisableResponse", 1);
            if (g_klassGameTimeManager)
            {
                g_miTimeGetDay = g_IL2CPP->class_get_method_from_name(g_klassGameTimeManager, "GetDay", 0);
                g_miTimeGetHour = g_IL2CPP->class_get_method_from_name(g_klassGameTimeManager, "GetHour", 0);
                g_miTimeGetTotalSeconds = g_IL2CPP->class_get_method_from_name(g_klassGameTimeManager, "GetTotalSeconds", 0);
                g_miTimeGetRemainHour = g_IL2CPP->class_get_method_from_name(g_klassGameTimeManager, "GetRemainHourFloat", 0);
                g_miTimeAddExtraCountDown = g_IL2CPP->class_get_method_from_name(g_klassGameTimeManager, "AddExtraCountDownTime", 1);
                g_miTimeNotifyScaleSync = g_IL2CPP->class_get_method_from_name(g_klassGameTimeManager, "NotifyCountDownTimerScaleSync", 0);
                g_miTimeSetClockFrozen = g_IL2CPP->class_get_method_from_name(g_klassGameTimeManager, "set_IsClockFrozen", 1);
            }
            if (g_klassCountDownTimer)
                g_miCdtGetRemainHour = g_IL2CPP->class_get_method_from_name(g_klassCountDownTimer, "GetRemainHourFloat", 0);
            if (g_klassToolset)
                g_miToolsetGameTimeFloat = g_IL2CPP->class_get_method_from_name(g_klassToolset, "GetGameTime_Float", 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            segOk = false;
            LOG_ERROR("ModBatch2 seg2 method resolve exception 0x%08X", GetExceptionCode());
        }
        // ===== 段3：RemoveBuff 重载区分（结构直读，独立保护，失败只影响移除） =====
        __try
        {
            // dump.cs 确认顺序：RemoveBuff(long) 在前、RemoveBuff(int) 在后
            g_miBuffRemoveByInstance = FindNthMethod(g_klassBuffComponent, "RemoveBuff", 1, 0); // long 版
            g_miBuffRemoveByConfig = FindNthMethod(g_klassBuffComponent, "RemoveBuff", 1, 1);  // int 版
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModBatch2 seg3 removebuff resolve exception 0x%08X", GetExceptionCode());
        }
        // ===== 段4：字段偏移动态解析（世界管理器/ShowWorld/Camera，免更偏移） =====
        __try
        {
            OFF_BLW_NeighborRescue = ResolveFieldOffset(g_klassBattleLogicWorld, "<_NeighborRescueManager>k__BackingField", OFF_BLW_NeighborRescue);
            OFF_BLW_ProficiencyManager = ResolveFieldOffset(g_klassBattleLogicWorld, "<_ProficiencyManager>k__BackingField", OFF_BLW_ProficiencyManager);
            OFF_BLW_SurvivalResultsManager = ResolveFieldOffset(g_klassBattleLogicWorld, "<_SurvivalResultsManager>k__BackingField", OFF_BLW_SurvivalResultsManager);
            OFF_BLW_ExploreManager = ResolveFieldOffset(g_klassBattleLogicWorld, "<_ExploreManager>k__BackingField", OFF_BLW_ExploreManager);
            OFF_BLW_AchievementManager = ResolveFieldOffset(g_klassBattleLogicWorld, "<_AchievementManager>k__BackingField", OFF_BLW_AchievementManager);
            OFF_BLW_CodexManager = ResolveFieldOffset(g_klassBattleLogicWorld, "<_CodexManager>k__BackingField", OFF_BLW_CodexManager);
            OFF_BSW_CameraManager = ResolveFieldOffset(g_klassBattleShowWorld, "<_CameraManager>k__BackingField", OFF_BSW_CameraManager);
            OFF_BSW_HotKeyManager = ResolveFieldOffset(g_klassBattleShowWorld, "<_HotKeyManager>k__BackingField", OFF_BSW_HotKeyManager);
            OFF_CAM_isKeyboardMoveBlocked = ResolveFieldOffset(g_klassCameraManager, "isKeyboardMoveBlocked", OFF_CAM_isKeyboardMoveBlocked);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModBatch2 seg4 offset resolve exception 0x%08X", GetExceptionCode());
        }
        LOG_INFO("ModBatch2 ready (segOk=%d removeCfg=%p removeInst=%p)", segOk ? 1 : 0, g_miBuffRemoveByConfig, g_miBuffRemoveByInstance);
        g_modBatch2Inited = true;
        return true;
    }


}

// ---------- 批次2 组件获取 ----------
static Il2CppObject* GetComp(Il2CppClass* klass)
{
    return GetLeadingRoleComponent(klass);
}

// BattleLogicWorld 字段取管理器（mod 同款：world._XxxManager，Proficiency/Explore/SurvivalResults/Neighbor/Achievement/Codex 都在这里）
static Il2CppObject* GetWorldManager(size_t fieldOffset)
{
    __try
    {
        BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
        MethodInfo* mi = FindMethodInHierarchy(g_klassBattleLogicWorld, "get_Instance", 0);
		//LOG_DEBUG("GetWorldManager offset=0x%zX world=%p get_Instance:%p", fieldOffset, world, mi ); 
        if (!world)
            return nullptr;
        return *(Il2CppObject**)((uint8_t*)world + fieldOffset);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

// Buff 名（Name_Local 优先）
static void BuffName(void* config, char* buf, size_t len, int32_t cfgId)
{
    buf[0] = 0;
    Config_Buff_o* cb = (Config_Buff_o*)config;
    if (cb)
    {
        char tmp[512];
        ILStringToUtf8(cb->Name_Local, tmp, sizeof(tmp));
        if (!tmp[0])
            ILStringToUtf8(cb->Name, tmp, sizeof(tmp));
        if (tmp[0])
        {
            // 防止拷贝溢出
            size_t n = strlen(tmp);
            if (n >= len)
                n = len - 1;
            memcpy(buf, tmp, n);
            buf[n] = 0;
            return;
        }
    }
    snprintf(buf, len, "Buff #%d", cfgId);
}

// 生存规划：已激活 id 集合（用于 Buff 移除跳过）
static bool PlanIdInSaveCache(int32_t talentId)
{
    __try
    {
        SurvivalPlanningComponent_o* sp = (SurvivalPlanningComponent_o*)GetComp(g_klassSurvivalPlanning);
        if (!sp || !sp->SaveCache)
            return false;
        size_t n = ListGetCount(sp->SaveCache);
        for (size_t i = 0; i < n; i++)
        {
            if (IntListGet(sp->SaveCache, i, -1) == talentId)
                return true;
        }
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// 查天赋配置（mod FindTalentConfig：Get_Config_Talent -> GetTalentConfig(id,1)）
static Config_Talent_o* FindTalentConfig(int32_t talentId)
{
    if (talentId <= 0)
        return nullptr;
    __try
    {
        Il2CppObject* cm = GetConfigManager();
        if (!cm)
            return nullptr;
        // 字典直读优先（_Config_Talent_Dict；免方法调用，Get_Config_Talent 运行时调用不稳）
        void* tld = *(void**)((uint8_t*)cm + OFF_CM_TalentDict);
        if (tld)
        {
            Config_Talent_o* cfg = (Config_Talent_o*)DictFindByIntKey(tld, talentId);
            if (cfg)
                return cfg;
        }
        if (g_miConfigGetTalent)
        {
            Config_Talent_o* cfg = (Config_Talent_o*)InvokeIntArg(g_miConfigGetTalent, cm, talentId);
            if (cfg)
                return cfg;
        }
        if (g_miConfigGetTalent2)
        {
            void* p[2] = { &talentId };
            int32_t one = 1;
            p[1] = &one;
            Config_Talent_o* cfg = (Config_Talent_o*)InvokeRet(g_miConfigGetTalent2, cm, p);
            if (cfg)
                return cfg;
        }
        return nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

// 生存规划 talent 对应的 BuffId（0 = 无）
static int32_t TalentBuffId(int32_t talentId)
{
    Config_Talent_o* cfg = FindTalentConfig(talentId);
    return cfg ? cfg->BuffID : 0;
}

// 生存规划 id 列表 -> 是否包含 configId（buff configId 匹配）
static bool ActivePlanHasBuffId(int32_t buffConfigId)
{
    __try
    {
        SurvivalPlanningComponent_o* sp = (SurvivalPlanningComponent_o*)GetComp(g_klassSurvivalPlanning);
        if (!sp || !sp->SaveCache)
            return false;
        size_t n = ListGetCount(sp->SaveCache);
        for (size_t i = 0; i < n; i++)
        {
            int32_t tid = IntListGet(sp->SaveCache, i, -1);
            if (tid > 0 && TalentBuffId(tid) == buffConfigId)
                return true;
        }
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- Buff ----------
int32_t SLSDK_GetBuffs(SLBuffView* outItems, int32_t maxItems)
{
    if (!ModBatch2Init())
        return 0;
    int32_t written = 0;
    __try
    {
        BuffComponent_o* buff = (BuffComponent_o*)GetComp(g_klassBuffComponent);
        if (!buff)
            return 0;
        // 1. buffArgsMap 字段（+0x48）直读：Dictionary<long, BuffArgs>
        //    AddBuff 写入的表（mod GetBuffsLegacy 的 GetEditorBuffMap 同数据源）
        void* map = buff->buffArgsMap;
        if (map)
        {
            Dictionary_o* d = (Dictionary_o*)map;
            if (d->_entries && d->_count > 0 && d->_count <= 100000)
            {
                uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
                for (int32_t i = 0; i < d->_count; i++)
                {
                    uint8_t* entry = entries + (size_t)i * 24;
                    int64_t key = *(int64_t*)(entry + 8);
                    void* val = *(void**)(entry + 16);
                    if (!val)
                        continue;
                    BuffArgs_o* ba = (BuffArgs_o*)val;
                    if (!ba->config)
                        continue;
                    Config_Buff_o* cb = (Config_Buff_o*)ba->config;
                    if (outItems && written < maxItems)
                    {
                        SLBuffView& v = outItems[written];
                        v.InstanceId = key;
                        v.ConfigId = cb->ID;
                        BuffName(cb, v.Name, sizeof(v.Name), cb->ID);
                        v.IsGood = cb->IsGood;
                        v.Layers = ba->BuffCount;
                        v.TimeEndTime = ba->TimeEndTime;
                    }
                    written++;
                }
            }
        }
        // 2. effectiveList 字段（+0x50）补充（按 InstanceId 去重）
        void* list = buff->effectiveList;
        if (list)
        {
            size_t n = ListGetCount(list);
            for (size_t i = 0; i < n; i++)
            {
                BuffArgs_o* ba = (BuffArgs_o*)ListGetItem(list, i);
                if (!ba || !ba->config)
                    continue;
                Config_Buff_o* cb = (Config_Buff_o*)ba->config;
                int64_t iid = ba->EffectBtArgsID;
                if (iid <= 0)
                    iid = ba->buffShowInstanceId;
                bool dup = false;
                for (int32_t j = 0; j < written && !dup; j++)
                {
                    if (iid > 0 && outItems && outItems[j].InstanceId == iid)
                        dup = true;
                    else if (iid <= 0 && outItems && outItems[j].ConfigId == cb->ID)
                        dup = true;
                }
                if (dup)
                    continue;
                if (outItems && written < maxItems)
                {
                    SLBuffView& v = outItems[written];
                    v.InstanceId = iid;
                    v.ConfigId = cb->ID;
                    BuffName(cb, v.Name, sizeof(v.Name), cb->ID);
                    v.IsGood = cb->IsGood;
                    v.Layers = ba->BuffCount;
                    v.TimeEndTime = ba->TimeEndTime;
                }
                written++;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return written;
}

int32_t SLSDK_GetBuffConfigs(SLBuffConfigView* outItems, int32_t maxItems)
{
    if (!ModBatch2Init())
        return 0;
    int32_t written = 0;
    __try
    {
        Il2CppObject* cm = GetConfigManager();
        if (!cm)
            return 0;
        void* dict = *(void**)((uint8_t*)cm + OFF_CM_BuffDict); // _Config_Buff_Dict
        if (!dict)
            return 0;
        Dictionary_o* d = (Dictionary_o*)dict;
        if (!d->_entries || d->_count <= 0 || d->_count > 100000)
            return 0;
        uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
        for (int32_t i = 0; i < d->_count; i++)
        {
            uint8_t* entry = entries + (size_t)i * 24;
            void* val = *(void**)(entry + 16);
            if (!val)
                continue;
            Config_Buff_o* cb = (Config_Buff_o*)val;
            if (outItems && written < maxItems)
            {
                SLBuffConfigView& v = outItems[written];
                v.ConfigId = cb->ID;
                BuffName(cb, v.Name, sizeof(v.Name), cb->ID);
                v.IsGood = cb->IsGood;
                v.Duration = cb->BuffDuring;
            }
            written++;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return written;
}

bool SLSDK_AddBuff(int32_t configId)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* buff = GetComp(g_klassBuffComponent);
        Il2CppObject* cm = GetConfigManager();
        if (!buff || !cm || !g_miConfigGetBuff || !g_miBuffAddBuff)
            return false;
        if (!InvokeIntArg(g_miConfigGetBuff, cm, configId))
            return false; // 配置不存在
        int64_t playerId = GetPlayerId();
        void* p[2] = { &playerId, &configId };
        if (!InvokeOk(g_miBuffAddBuff, buff, p))
            return false;
        if (g_miBuffRequestRefresh)
            InvokeNoArg(g_miBuffRequestRefresh, buff);
        // 验证：GetBuffConfigIds 应包含 configId（mod AddBuff 同款校验）
        if (g_miBuffGetBuffConfigIds)
        {
            Il2CppObject* ids = InvokeNoArg(g_miBuffGetBuffConfigIds, buff);
            if (ids)
            {
                size_t n = ListGetCount(ids);
                for (size_t i = 0; i < n; i++)
                {
                    if (IntListGet(ids, i, -1) == configId)
                        return true;
                }
                return false; // 游戏未接受该 Buff
            }
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_RemoveBuffByConfig(int32_t configId)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        // 生存规划来源 -> 移除生存规划
        if (ActivePlanHasBuffId(configId))
        {
            SurvivalPlanningComponent_o* sp = (SurvivalPlanningComponent_o*)GetComp(g_klassSurvivalPlanning);
            if (sp && sp->SaveCache)
            {
                size_t n = ListGetCount(sp->SaveCache);
                for (size_t i = 0; i < n; i++)
                {
                    int32_t tid = IntListGet(sp->SaveCache, i, -1);
                    if (tid > 0 && TalentBuffId(tid) == configId)
                        return SLSDK_RemoveSurvivalPlan(tid);
                }
            }
        }
        Il2CppObject* buff = GetComp(g_klassBuffComponent);
        if (!buff || !g_miBuffRemoveByConfig)
            return false;
        void* p[1] = { &configId };
        bool ok = InvokeOk(g_miBuffRemoveByConfig, buff, p);
        if (ok && g_miBuffRequestRefresh)
            InvokeNoArg(g_miBuffRequestRefresh, buff);
        return ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_RemoveBuff(int64_t instanceId)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* buff = GetComp(g_klassBuffComponent);
        if (!buff || !g_miBuffRemoveByInstance || instanceId <= 0)
            return false;
        void* p[1] = { &instanceId };
        bool ok = InvokeOk(g_miBuffRemoveByInstance, buff, p);
        if (ok && g_miBuffRequestRefresh)
            InvokeNoArg(g_miBuffRequestRefresh, buff);
        return ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_ClearAllBuffs()
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* buff = GetComp(g_klassBuffComponent);
        if (!buff || !g_miBuffGetBuffConfigIds || !g_miBuffRemoveByConfig)
            return false;
        Il2CppObject* ids = InvokeNoArg(g_miBuffGetBuffConfigIds, buff);
        if (!ids)
            return false;
        size_t n = ListGetCount(ids);
        bool all = true;
        for (size_t i = 0; i < n; i++)
        {
            int32_t cfgId = IntListGet(ids, i, -1);
            if (cfgId <= 0 || ActivePlanHasBuffId(cfgId))
                continue; // 跳过生存规划
            void* p[1] = { &cfgId };
            if (!InvokeOk(g_miBuffRemoveByConfig, buff, p))
                all = false;
        }
        if (g_miBuffRequestRefresh)
            InvokeNoArg(g_miBuffRequestRefresh, buff);
        return all;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

int32_t SLSDK_RemoveAllNegativeBuffs()
{
    if (!ModBatch2Init())
        return 0;
    int32_t removed = 0;
    __try
    {
        Il2CppObject* buff = GetComp(g_klassBuffComponent);
        Il2CppObject* cm = GetConfigManager();
        if (!buff || !cm || !g_miBuffGetBuffConfigIds || !g_miBuffRemoveByConfig)
            return 0;
        // 收集负面 buff id
        void* dict = *(void**)((uint8_t*)cm + OFF_CM_BuffDict); // _Config_Buff_Dict
        if (!dict)
            return 0;
        Dictionary_o* d = (Dictionary_o*)dict;
        if (!d->_entries || d->_count <= 0 || d->_count > 100000)
            return 0;
        uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
        // 简单：直接在遍历配置时对每个负面 buff 尝试移除（GetBuffConfigIds 校验会做？不，直接移除）
        for (int32_t i = 0; i < d->_count; i++)
        {
            uint8_t* entry = entries + (size_t)i * 24;
            void* val = *(void**)(entry + 16);
            if (!val)
                continue;
            Config_Buff_o* cb = (Config_Buff_o*)val;
            if (cb->IsGood || ActivePlanHasBuffId(cb->ID))
                continue;
            void* p[1] = { &cb->ID };
            if (InvokeOk(g_miBuffRemoveByConfig, buff, p))
                removed++;
        }
        if (removed > 0 && g_miBuffRequestRefresh)
            InvokeNoArg(g_miBuffRequestRefresh, buff);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return removed;
}

// ---------- 生存规划 ----------
int32_t SLSDK_GetSurvivalPlans(SLSurvivalPlanView* outItems, int32_t maxItems)
{
    if (!ModBatch2Init())
        return 0;
    int32_t written = 0;
    __try
    {
        SurvivalPlanningComponent_o* sp = (SurvivalPlanningComponent_o*)GetComp(g_klassSurvivalPlanning);
        if (!sp || !sp->SaveCache)
            return 0;
        size_t n = ListGetCount(sp->SaveCache);
        for (size_t i = 0; i < n; i++)
        {
            int32_t tid = IntListGet(sp->SaveCache, i, -1);
            if (tid <= 0)
                continue;
            Config_Talent_o* cfg = FindTalentConfig(tid);
            if (outItems && written < maxItems)
            {
                SLSurvivalPlanView& v = outItems[written];
                v.TalentId = tid;
                v.Level = cfg ? cfg->Lv : 0;
                v.Active = true;
                char tmp[512];
                ILStringToUtf8(cfg ? cfg->Name_Local : nullptr, tmp, sizeof(tmp));
                if (!tmp[0])
                    ILStringToUtf8(cfg ? cfg->Name : nullptr, tmp, sizeof(tmp));
                if (!tmp[0])
                    snprintf(tmp, sizeof(tmp), "生存规划 #%d", tid);
                size_t n2 = strlen(tmp);
                if (n2 >= sizeof(v.Name))
                    n2 = sizeof(v.Name) - 1;
                memcpy(v.Name, tmp, n2);
                v.Name[n2] = 0;
                ILStringToUtf8(cfg ? cfg->Dec_Local : nullptr, tmp, sizeof(tmp));
                if (!tmp[0])
                    ILStringToUtf8(cfg ? cfg->Dec : nullptr, tmp, sizeof(tmp));
                n2 = strlen(tmp);
                if (n2 >= sizeof(v.Description))
                    n2 = sizeof(v.Description) - 1;
                memcpy(v.Description, tmp, n2);
                v.Description[n2] = 0;
            }
            written++;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return written;
}

int32_t SLSDK_GetSurvivalPlanCatalog(SLSurvivalPlanView* outItems, int32_t maxItems)
{
    if (!ModBatch2Init())
        return 0;
    int32_t written = 0;
    __try
    {
        Il2CppObject* cm = GetConfigManager();
        SurvivalPlanningComponent_o* sp = (SurvivalPlanningComponent_o*)GetComp(g_klassSurvivalPlanning);
        if (!cm)
            return 0;
        // 全量天赋字典直读（和 Buff 目录遍历 _Config_Buff_Dict 对称；mod 目录语义 = 列出全部可添加生存规划）
        void* dict = *(void**)((uint8_t*)cm + OFF_CM_TalentDict); // ConfigManager._Config_Talent_Dict
        if (!dict)
            return 0;
        Dictionary_o* d = (Dictionary_o*)dict;
        if (!d->_entries || d->_count <= 0 || d->_count > 100000)
            return 0;
        uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
        for (int32_t i = 0; i < d->_count; i++)
        {
            uint8_t* entry = entries + (size_t)i * 24;
            int32_t tid = *(int32_t*)(entry + 8); // 字典 key = talent id（AddSurvivalPlan 同参数）
            void* val = *(void**)(entry + 16);
            if (!val || tid <= 0)
                continue;
            Config_Talent_o* cfg = (Config_Talent_o*)val;
            // 填充必须包在 outItems 检查内（计数调用 nullptr,0 不能写 outItems；对齐 GetBuffConfigs）
            if (outItems && written < maxItems)
            {
                SLSurvivalPlanView& v = outItems[written];
                v.TalentId = tid;
                v.Level = cfg->Lv;
                v.Active = sp && sp->SaveCache ? PlanIdInSaveCache(tid) : false;
                char tmp[512];
                ILStringToUtf8(cfg->Name_Local, tmp, sizeof(tmp));
                if (!tmp[0])
                    ILStringToUtf8(cfg->Name, tmp, sizeof(tmp));
                if (!tmp[0])
                    snprintf(tmp, sizeof(tmp), "生存规划 #%d", tid);
                size_t n2 = strlen(tmp);
                if (n2 >= sizeof(v.Name))
                    n2 = sizeof(v.Name) - 1;
                memcpy(v.Name, tmp, n2);
                v.Name[n2] = 0;
                ILStringToUtf8(cfg->Dec_Local, tmp, sizeof(tmp));
                if (!tmp[0])
                    ILStringToUtf8(cfg->Dec, tmp, sizeof(tmp));
                n2 = strlen(tmp);
                if (n2 >= sizeof(v.Description))
                    n2 = sizeof(v.Description) - 1;
                memcpy(v.Description, tmp, n2);
                v.Description[n2] = 0;
            }
            written++;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return written;
}

bool SLSDK_AddSurvivalPlan(int32_t talentId)
{
    g_planError[0] = 0;
    g_planStep = "ModBatch2Init";
    if (!ModBatch2Init())
    {
        snprintf(g_planError, sizeof(g_planError), "ModBatch2Init 失败");
        return false;
    }
    SurvivalPlanningComponent_o* sp = nullptr;
    Il2CppObject* buff = nullptr;
    __try
    {
        // 确保当前线程已 attach il2cpp（面板在 D3D11 Present 渲染线程执行，未 attach 时写操作 runtime_invoke 会访问违例）
        if (g_domain && g_IL2CPP && g_IL2CPP->thread_attach)
            g_IL2CPP->thread_attach(g_domain);
        g_planStep = "GetComp SP";
        sp = (SurvivalPlanningComponent_o*)GetComp(g_klassSurvivalPlanning);
        g_planStep = "GetComp Buff";
        buff = GetComp(g_klassBuffComponent);
        if (!sp)
        {
            snprintf(g_planError, sizeof(g_planError), "SurvivalPlanningComponent 为空");
            return false;
        }
        if (!buff)
        {
            snprintf(g_planError, sizeof(g_planError), "BuffComponent 为空");
            return false;
        }
        if (talentId <= 0)
        {
            snprintf(g_planError, sizeof(g_planError), "无效 talentId=%d", talentId);
            return false;
        }
        g_planStep = "FindTalentConfig";
        if (!FindTalentConfig(talentId))
        {
            snprintf(g_planError, sizeof(g_planError), "Talent 配置不存在 #%d", talentId);
            return false;
        }
        g_planStep = "SaveCache 检查";
        if (!sp->SaveCache)
        {
            snprintf(g_planError, sizeof(g_planError), "SaveCache 未加载");
            return false;
        }
        g_planStep = "Invoke AddBuff";
        // 传参约定：引用类型参数直接传对象指针（IL2CPP runtime_invoke 对引用参数按值传递对象引用）
        void* p[2] = { buff, &talentId };
        if (!InvokeOk(g_miPlanAddBuff, sp, p))
        {
            snprintf(g_planError, sizeof(g_planError), "AddBuff 调用异常（方法=%p）", (void*)g_miPlanAddBuff);
            return false;
        }
        g_planStep = "NotifyUpdate";
        if (g_miPlanNotifyUpdate)
            InvokeNoArg(g_miPlanNotifyUpdate, sp);
        g_planStep = "BuffRefresh";
        if (g_miBuffRequestRefresh)
            InvokeNoArg(g_miBuffRequestRefresh, buff);
        g_planStep = "SaveCache 校验";
        if (!PlanIdInSaveCache(talentId))
        {
            snprintf(g_planError, sizeof(g_planError), "AddBuff 返回但 SaveCache 未包含 #%d", talentId);
            return false;
        }
        g_planStep = "完成";
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        snprintf(g_planError, sizeof(g_planError), "异常 0x%08X @ %s (sp=%p buff=%p mi=%p)", GetExceptionCode(), g_planStep ? g_planStep : "?", (void*)sp, (void*)buff, (void*)g_miPlanAddBuff);
        return false;
    }
}

// 上次生存规划操作失败原因
const char* SLSDK_GetLastPlanError()
{
    return g_planError;
}

bool SLSDK_RemoveSurvivalPlan(int32_t talentId)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        // 确保当前线程已 attach il2cpp（同 AddSurvivalPlan）
        if (g_domain && g_IL2CPP && g_IL2CPP->thread_attach)
            g_IL2CPP->thread_attach(g_domain);

        SurvivalPlanningComponent_o* sp = (SurvivalPlanningComponent_o*)GetComp(g_klassSurvivalPlanning);
        Il2CppObject* buff = GetComp(g_klassBuffComponent);
        if (!sp || !buff || talentId <= 0)
            return false;
        // 传参约定：引用类型参数直接传对象指针（同 AddSurvivalPlan）
        void* p[2] = { buff, &talentId };
        if (!InvokeOk(g_miPlanRemoveBuff, sp, p))
            return false;
        if (g_miPlanNotifyUpdate)
            InvokeNoArg(g_miPlanNotifyUpdate, sp);
        if (g_miBuffRequestRefresh)
            InvokeNoArg(g_miBuffRequestRefresh, buff);
        return !PlanIdInSaveCache(talentId);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- 熟练度 ----------
int32_t SLSDK_GetProficiencies(SLProficiencyView* outItems, int32_t maxItems)
{
    if (!ModBatch2Init())
        return 0;
    int32_t written = 0;
    __try
    {
        Il2CppObject* prof = GetWorldManager(OFF_BLW_ProficiencyManager);
        if (!prof || !g_miProfGetSnapshot)
            return 0;
        Il2CppObject* list = InvokeNoArg(g_miProfGetSnapshot, prof);
        if (!list)
            return 0;
        size_t n = ListGetCount(list);
        for (size_t i = 0; i < n; i++)
        {
            ProficiencySystemSnapshot_o* snap = (ProficiencySystemSnapshot_o*)ListGetItem(list, i);
            if (!snap || snap->SystemId < 1 || snap->SystemId > 6)
                continue;
            if (!(outItems && written < maxItems))
            {
                written++;
                continue;
            }
            SLProficiencyView& v = outItems[written];
            v.TypeId = snap->SystemId;
            char tmp[256];
            ILStringToUtf8(snap->SystemName, tmp, sizeof(tmp));
            if (!tmp[0])
            {
                // SystemName 为空时兜底 ProficiencyType 枚举名（1=制造 2=种植 3=烹饪 4=陷阱 5=探索 6=防御）
                static const char* prof_enum_names[6] = {
                    "制造 Production",
                    "种植 Plant",
                    "烹饪 Cooking",
                    "陷阱 Trap",
                    "探索 Explore",
                    "防御 Defense",
                };
                int32_t idx = snap->SystemId - 1;
                snprintf(tmp, sizeof(tmp), "%s", (idx >= 0 && idx < 6) ? prof_enum_names[idx] : "未知");
            }
            size_t n2 = strlen(tmp);
            if (n2 >= sizeof(v.Name))
                n2 = sizeof(v.Name) - 1;
            memcpy(v.Name, tmp, n2);
            v.Name[n2] = 0;
            v.Level = snap->CurrentLevel;
            v.Exp = snap->CurrentExp;
            v.PrevLevelExp = snap->PrevLevelExp;
            v.NextLevelExp = snap->NextLevelExp;
            v.MaxLevel = 5;
            written++;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return written;
}

bool SLSDK_AddProficiencyExp(int32_t typeId, int32_t amount)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        if (typeId < 1 || typeId > 6 || amount <= 0)
            return false;
        Il2CppObject* prof = GetWorldManager(OFF_BLW_ProficiencyManager);
        if (!prof || !g_miProfIsMaxLevel || !g_miProfAddExp)
            return false;
        void* p1[1] = { &typeId };
        bool maxed = UnboxBool((Il2CppObject*)InvokeRet(g_miProfIsMaxLevel, prof, p1), false);
        if (maxed)
            return false;
        void* nullStr = nullptr;
        int32_t zero = 0;
        int32_t src = 1; // ProficiencyExpSource=1
        void* p2[6] = { &typeId, &nullStr, &amount, &zero, &zero, &src };
        return InvokeOk(g_miProfAddExp, prof, p2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_AddProficiencyLevels(int32_t typeId, int32_t levels, int32_t* appliedOut)
{
    if (appliedOut)
        *appliedOut = 0;
    if (!ModBatch2Init())
        return false;
    __try
    {
        if (typeId < 1 || typeId > 6 || levels <= 0)
            return false;
        Il2CppObject* prof = GetWorldManager(OFF_BLW_ProficiencyManager);
        if (!prof || !g_miProfGetLevel || !g_miProfAddLevel)
            return false;
        void* p1[1] = { &typeId };
        int32_t level = UnboxInt32((Il2CppObject*)InvokeRet(g_miProfGetLevel, prof, p1), -1);
        if (level < 0)
            return false;
        int32_t applied = levels;
        int32_t room = 5 - level;
        if (applied > room)
            applied = room;
        if (applied <= 0)
            return false;
        uint8_t bFalse = 0;
        void* p2[3] = { &typeId, &applied, &bFalse };
        if (!InvokeOk(g_miProfAddLevel, prof, p2))
            return false;
        if (appliedOut)
            *appliedOut = applied;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- 邻居关系 ----------
int32_t GetNeighborAffinityMax()
{
    if (!g_klassGameKey || !g_IL2CPP)
        return 100;
    __try
    {
        FieldInfo* f = FindStaticField(g_klassGameKey, "_GlobalSetting_Neighbor_AffinityMax");
        if (!f)
            return 100;
        int32_t v = 0;
        g_IL2CPP->field_static_get_value(f, &v);
        return v > 0 ? v : 100;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 100;
    }
}

bool SLSDK_GetRelationship(SLRelationshipView* out)
{
    if (!out)
        return false;
    memset(out, 0, sizeof(*out));
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* nb = GetWorldManager(OFF_BLW_NeighborRescue);
        if (!nb || !g_miNeighborGetAffinity || !g_miNeighborGetTier || !g_miNeighborIsUnlocked)
            return false;
        out->Affinity = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miNeighborGetAffinity, nb), -1);
        out->Tier = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miNeighborGetTier, nb), -1);
        out->MaxAffinity = GetNeighborAffinityMax();
        out->Locked = !UnboxBool((Il2CppObject*)InvokeNoArg(g_miNeighborIsUnlocked, nb), false);
        snprintf(out->TierName, sizeof(out->TierName), "Tier %d", out->Tier);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_SetRelationship(int32_t value)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* nb = GetWorldManager(OFF_BLW_NeighborRescue);
        if (!nb || !g_miNeighborIsUnlocked || !g_miNeighborGetAffinity || !g_miNeighborAddAffinity)
            return false;
        bool unlocked = UnboxBool((Il2CppObject*)InvokeNoArg(g_miNeighborIsUnlocked, nb), false);
        if (!unlocked)
            return false;
        int32_t maxA = GetNeighborAffinityMax();
        if (value > maxA || value < 0)
            return false;
        int32_t cur = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miNeighborGetAffinity, nb), -1);
        if (cur < 0)
            return false;
        int32_t delta = value - cur;
        int64_t playerId = GetPlayerId();
        void* p[2] = { &delta, &playerId };
        if (!InvokeOk(g_miNeighborAddAffinity, nb, p))
            return false;
        int32_t after = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miNeighborGetAffinity, nb), -1);
        return after == value;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- 图鉴 / 成就 ----------
bool SLSDK_UnlockAllCodex()
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* codex = GetWorldManager(OFF_BLW_CodexManager);
        if (!codex || !g_miCodexUnlockAll)
            return false;
        //LOG_INFO("g_miCodexUnlockAll:%p", g_miCodexUnlockAll);//GameCore.HotUpdate.Battle.Logic.CodexManager.GmUnlockAll 
        return InvokeOk(g_miCodexUnlockAll, codex, nullptr);
       // return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_UnlockAllAchievements()
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* am = GetWorldManager(OFF_BLW_AchievementManager);
        if (!am || !g_miAchieveUnlockAll)
            return false;
        return InvokeOk(g_miAchieveUnlockAll, am, nullptr);
        //LOG_INFO("g_miAchieveUnlockAll:%p", g_miAchieveUnlockAll);//GameCore.HotUpdate.Battle.Logic.AchievementManager.GmUnlockAll 

        //return false;   
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- 暴露度 ----------
bool SLSDK_GetExposure(float* current, int32_t* maximum, float* timeExposure, float* moveExposure, bool* running)
{
    if (current)
        *current = 0.0f;
    if (maximum)
        *maximum = 0;
    if (timeExposure)
        *timeExposure = 0.0f;
    if (moveExposure)
        *moveExposure = 0.0f;
    if (running)
        *running = false;
    if (!ModBatch2Init())
        return false;
    __try
    {
        ExploreManager_o* ex = (ExploreManager_o*)GetWorldManager(OFF_BLW_ExploreManager);
        if (!ex)
            return false;
        if (current)
            *current = ex->CurExposure;
        if (maximum)
            *maximum = g_miExploreGetMax ? UnboxInt32((Il2CppObject*)InvokeNoArg(g_miExploreGetMax, ex), 0) : 0;
        if (timeExposure)
            *timeExposure = ex->TimeExposure;
        if (moveExposure)
            *moveExposure = ex->MoveExposure;
        if (running)
            *running = ex->IsRunning;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_SetExposure(float value)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        ExploreManager_o* ex = (ExploreManager_o*)GetWorldManager(OFF_BLW_ExploreManager);
        if (!ex)
            return false;
        int32_t maxA = g_miExploreGetMax ? UnboxInt32((Il2CppObject*)InvokeNoArg(g_miExploreGetMax, ex), 0) : 0;
        float clamped = value;
        if (clamped < 0.0f)
            clamped = 0.0f;
        if (maxA > 0 && clamped > (float)maxA)
            clamped = (float)maxA;
        ex->CurExposure = clamped;
        if (g_miExploreNotifyUI)
            InvokeNoArg(g_miExploreNotifyUI, ex);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_SetNoExploreExposure(bool enabled)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        if (g_klassWebGm && g_IL2CPP)
        {
            FieldInfo* f = FindStaticField(g_klassWebGm, "<LockExploreExposure>k__BackingField");
            if (f)
            {
                bool v = enabled;
                g_IL2CPP->field_static_set_value(f, &v);
            }
        }
        g_noExploreExposure = enabled;
        if (enabled)
        {
            ExploreManager_o* ex = (ExploreManager_o*)GetWorldManager(OFF_BLW_ExploreManager);
            if (ex)
            {
                ex->CurExposure = 0.0f;
                if (g_miExploreNotifyUI)
                    InvokeNoArg(g_miExploreNotifyUI, ex);
            }
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void SLSDK_ApplyNoExploreExposure()
{
    if (!g_noExploreExposure)
        return;
    if (!ModBatch2Init())
        return;
    __try
    {
        ExploreManager_o* ex = (ExploreManager_o*)GetWorldManager(OFF_BLW_ExploreManager);
        if (ex && ex->CurExposure > 0.0f)
        {
            ex->CurExposure = 0.0f;
            if (g_miExploreNotifyUI)
                InvokeNoArg(g_miExploreNotifyUI, ex);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// ---------- 生存点 ----------
int32_t SLSDK_GetSurvivalPoints()
{
    if (!ModBatch2Init())
        return -1;
    __try
    {
        Il2CppObject* sr = GetWorldManager(OFF_BLW_SurvivalResultsManager);
        if (!sr || !g_miSurvResultGetPoint)
            return -1;
        return UnboxInt32((Il2CppObject*)InvokeNoArg(g_miSurvResultGetPoint, sr), -1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

bool SLSDK_SetSurvivalPoints(int32_t value)
{
    if (!ModBatch2Init())
        return false;
    __try
    {
        Il2CppObject* sr = GetWorldManager(OFF_BLW_SurvivalResultsManager);
        if (!sr || !g_miSurvResultGetPoint || !g_miSurvResultAddPoint)
            return false;
        int32_t cur = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miSurvResultGetPoint, sr), -1);
        if (cur < 0)
            return false;
        int32_t delta = value - cur;
        void* p[1] = { &delta };
        if (!InvokeOk(g_miSurvResultAddPoint, sr, p))
            return false;
        int32_t after = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miSurvResultGetPoint, sr), -1);
        return after == value;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- 移动 / 热键 ----------
bool SLSDK_SetMovementBlocked(bool blocked)
{
    __try
    {
        Il2CppObject* show = GetSingletonInstance(g_klassBattleShowWorld);
        if (!show)
            return false;
        void* cam = *(void**)((uint8_t*)show + OFF_BSW_CameraManager); // _CameraManager
        if (!cam)
            return false;
        *(bool*)((uint8_t*)cam + OFF_CAM_isKeyboardMoveBlocked) = blocked; // isKeyboardMoveBlocked
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void SLSDK_SetHotKeyDisabled(bool disabled)
{
    __try
    {
        Il2CppObject* show = GetSingletonInstance(g_klassBattleShowWorld);
        if (!show || !g_miHotKeyDisable)
            return;
        void* hk = *(void**)((uint8_t*)show + OFF_BSW_HotKeyManager); // _HotKeyManager
        if (!hk)
            return;
        uint8_t v = disabled ? 1 : 0;
        void* p[1] = { &v };
        InvokeOk(g_miHotKeyDisable, hk, p);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// ---------- 时间 ----------
static GameTimeManager_o* GetTimeManager()
{
    __try
    {
        BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
        return (world && world->_GameTimeManager) ? world->_GameTimeManager : nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

int32_t SLSDK_GameDay()
{
    if (!ModBatch2Init())
        return -1;
    __try
    {
        GameTimeManager_o* gtm = GetTimeManager();
        return (gtm && g_miTimeGetDay) ? UnboxInt32((Il2CppObject*)InvokeNoArg(g_miTimeGetDay, gtm), -1) : -1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

int32_t SLSDK_GameHour()
{
    if (!ModBatch2Init())
        return -1;
    __try
    {
        GameTimeManager_o* gtm = GetTimeManager();
        return (gtm && g_miTimeGetHour) ? UnboxInt32((Il2CppObject*)InvokeNoArg(g_miTimeGetHour, gtm), -1) : -1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

int32_t SLSDK_GameTotalSeconds()
{
    if (!ModBatch2Init())
        return -1;
    __try
    {
        GameTimeManager_o* gtm = GetTimeManager();
        return (gtm && g_miTimeGetTotalSeconds) ? UnboxInt32((Il2CppObject*)InvokeNoArg(g_miTimeGetTotalSeconds, gtm), -1) : -1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

bool SLSDK_IsClockFrozen()
{
    GameTimeManager_o* gtm = GetTimeManager();
    return g_frozenOverride || (gtm ? gtm->_IsClockFrozen : false);
}

float SLSDK_RemainCountdownHour()
{
    if (!ModBatch2Init())
        return -1.0f;
    __try
    {
        GameTimeManager_o* gtm = GetTimeManager();
        return (gtm && g_miTimeGetRemainHour) ? UnboxFloat((Il2CppObject*)InvokeNoArg(g_miTimeGetRemainHour, gtm), -1.0f) : -1.0f;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1.0f;
    }
}

bool SLSDK_ExtendCountdown(int32_t hours)
{
    if (!ModBatch2Init() || hours <= 0)
        return false;
    __try
    {
        GameTimeManager_o* gtm = GetTimeManager();
        if (!gtm || !gtm->_Timer)
            return false;
        // 仅 CountDownTimer（准备阶段）
        Il2CppClass* k = g_IL2CPP->object_get_class((Il2CppObject*)gtm->_Timer);
        if (k != g_klassCountDownTimer)
            return false;
        CountDownTimer_o* cdt = (CountDownTimer_o*)gtm->_Timer;
        if (!g_miTimeAddExtraCountDown || !g_miCdtGetRemainHour)
            return false;
        float remainBefore = UnboxFloat((Il2CppObject*)InvokeNoArg(g_miCdtGetRemainHour, cdt), -1.0f);
        if (remainBefore < 0.0f)
            return false;
        int32_t num = hours * 3600;
        void* p1[1] = { &num };
        InvokeOk(g_miTimeAddExtraCountDown, gtm, p1);
        float remainAfter = UnboxFloat((Il2CppObject*)InvokeNoArg(g_miCdtGetRemainHour, cdt), -1.0f);
        if (remainAfter <= remainBefore + 0.0001f)
        {
            // fallback：Toolset.GetGameTime_Float(num) 换算 + 直写字段
            float delta = 0.0f;
            if (g_miToolsetGameTimeFloat)
            {
                void* p2[1] = { &num };
                delta = UnboxFloat((Il2CppObject*)InvokeRet(g_miToolsetGameTimeFloat, nullptr, p2), 0.0f);
            }
            if (!(delta > 0.0f))
                return false;
            float oldRemain = cdt->_RemainTime;
            float oldTotal = cdt->_TotalTime;
            int32_t oldExtra = cdt->_ExtraCountDownTimeConfig;
            cdt->_RemainTime = oldRemain + delta;
            cdt->_TotalTime = oldTotal + delta;
            cdt->_IsCompleted = false;
            if (cdt->_ExtraCountDownTimeConfig <= oldExtra)
                cdt->_ExtraCountDownTimeConfig = oldExtra + num;
            if (g_miTimeAddExtraCountDown)
            {
                int32_t zero = 0;
                void* p3[1] = { &zero };
                InvokeOk(g_miTimeAddExtraCountDown, gtm, p3);
            }
        }
        if (g_miTimeNotifyScaleSync)
            InvokeNoArg(g_miTimeNotifyScaleSync, gtm);
        float finalRemain = UnboxFloat((Il2CppObject*)InvokeNoArg(g_miCdtGetRemainHour, cdt), -1.0f);
        return finalRemain > remainBefore;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_SetTimeFrozen(bool on)
{
    GameTimeManager_o* gtm = GetTimeManager();
    if (gtm)
    {
        if (g_miTimeSetClockFrozen)
        {
            // mod 同款 time.IsClockFrozen = on（走 setter，可能有同步 UI 等副作用）
            uint8_t v = on ? 1 : 0;
            void* p[1] = { &v };
            InvokeOk(g_miTimeSetClockFrozen, gtm, p);
        }
        else
        {
            gtm->_IsClockFrozen = on;
        }
    }
    g_frozenOverride = on;
    LOG_INFO("[Freeze] SetTimeFrozen(%d) gtm=%p frozenOverride=%d setter=%p", on ? 1 : 0, gtm, g_frozenOverride ? 1 : 0, g_miTimeSetClockFrozen);
    return true;
}

void SLSDK_ApplyFrozenOverride()
{
    if (!g_frozenOverride)
        return;
    GameTimeManager_o* gtm = GetTimeManager();
    if (!gtm || gtm->_IsClockFrozen)
        return;
    if (g_miTimeSetClockFrozen)
    {
        uint8_t v = 1;
        void* p[1] = { &v };
        InvokeOk(g_miTimeSetClockFrozen, gtm, p);
    }
    else
    {
        gtm->_IsClockFrozen = true;
    }
}

// ============================================================
// 批次3：设施耐久 / 无限食物开关 / 背包尺寸 / 负重（mod 迁移）
// ============================================================

// Config_Bag（背包配置）
struct Config_Bag_o : Il2CppObject
{
    int32_t ID;          // +0x10
    void* Name;          // +0x18
    void* Name_Local;    // +0x20
    void* Size;          // +0x28 List<int>
    int32_t Burden;      // +0x30
};

// FurnitureDurabilityComponent（BaseComponent 之后）
struct FurnitureDurabilityComponent_o : BaseComponent_o
{
    int32_t _crisisId;           // +0x40
    int32_t CurrentDurability;   // +0x44
    int32_t MaxDurability;       // +0x48
};

namespace
{
    Il2CppClass* g_klassConfigBag = nullptr;
    Il2CppClass* g_klassFurnitureDurability = nullptr;
    MethodInfo* g_miConfigGetBag = nullptr;          // Get_Config_Bag(int)
    MethodInfo* g_miAgentGetHomeFurnitures = nullptr; // GetHomeFurnituresBySlotType(int)
    MethodInfo* g_miFurnSetMaxDurability = nullptr;   // SetMaxDurability(int)
    MethodInfo* g_miFurnSetCurDurability = nullptr;   // SetCurrentDurability(int)
    MethodInfo* g_miFurnGetCurDurability = nullptr;   // GetCurrentDurability()
    MethodInfo* g_miFurnGetMaxDurability = nullptr;   // GetMaxDurability()
    MethodInfo* g_miBagGetConfigId = nullptr;         // GetBagConfigId()
    MethodInfo* g_miBagGetWeight = nullptr;           // GetBagWeight()
    MethodInfo* g_miBagGetMaxBurden = nullptr;        // GetMaxBurden()
    MethodInfo* g_miBagAddExtraBurden = nullptr;      // AddExtraBurden(int)
    MethodInfo* g_miBagRemoveExtraBurden = nullptr;   // RemoveExtraBurden(int)
    MethodInfo* g_miItemsRefreshTimeScale = nullptr;  // RefreshAllItemTimeScale()
    bool g_modBatch3Inited = false;
    bool g_infiniteFoodShelfLife = false;             // mod InfiniteFoodShelfLife
    int32_t g_appliedExtraBurden = 0;                 // 负重补足量（mod _appliedExtraBurden）
    int32_t g_bagSizeCols = 0, g_bagSizeRows = 0;     // 背包原始尺寸记忆（mod OriginalSizes）
    int32_t g_desiredBagCols = 0, g_desiredBagRows = 0; // 期望背包尺寸（mod _desiredColumns/_desiredRows，hook 用）
    // 物品柜/架容量增强（懒虫增强版白名单扩容）：收纳架/置物架/冰箱白名单 + 行数/负重倍率
    int32_t g_containerRowsMult = 1;   // 柜子行数倍率（1=关闭）
    int32_t g_containerBurdenMult = 1; // 柜子负重倍率（1=关闭）
    std::unordered_set<int32_t> g_containerBagIds;            // 白名单 bagId（名字匹配后加入）
    std::map<void*, int32_t> g_origContainerRows;   // 原始行数记忆（per Config_Bag 对象指针）
    std::map<void*, int32_t> g_origContainerBurden; // 原始负重记忆

    static bool ModBatch3Init()
    {
        if (g_modBatch3Inited)
            return true;
        if (!ModItemsInit() || !ModBatch2Init())
            return false;
        __try
        {
            g_klassConfigBag = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_Bag");
            g_klassFurnitureDurability = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "FurnitureDurabilityComponent");
            if (!g_klassConfigBag || !g_klassFurnitureDurability)
            {
                LOG_ERROR("ModBatch3 class NULL: ConfigBag=%p FurnDur=%p", g_klassConfigBag, g_klassFurnitureDurability);
                return false;
            }
            if (g_klassConfigManager)
            {
                g_miConfigGetBag = g_IL2CPP->class_get_method_from_name(g_klassConfigManager, "Get_Config_Bag", 1);
                g_miAgentGetHomeFurnitures = g_IL2CPP->class_get_method_from_name(g_klassAgentManager, "GetHomeFurnituresBySlotType", 1);
            }
            if (g_klassFurnitureDurability)
            {
                g_miFurnSetMaxDurability = g_IL2CPP->class_get_method_from_name(g_klassFurnitureDurability, "SetMaxDurability", 1);
                g_miFurnSetCurDurability = g_IL2CPP->class_get_method_from_name(g_klassFurnitureDurability, "SetCurrentDurability", 1);
                g_miFurnGetCurDurability = g_IL2CPP->class_get_method_from_name(g_klassFurnitureDurability, "GetCurrentDurability", 0);
                g_miFurnGetMaxDurability = g_IL2CPP->class_get_method_from_name(g_klassFurnitureDurability, "GetMaxDurability", 0);
            }
            if (g_klassBagComponent)
            {
                g_miBagGetConfigId = g_IL2CPP->class_get_method_from_name(g_klassBagComponent, "GetBagConfigId", 0);
                g_miBagGetWeight = g_IL2CPP->class_get_method_from_name(g_klassBagComponent, "GetBagWeight", 0);
                g_miBagGetMaxBurden = g_IL2CPP->class_get_method_from_name(g_klassBagComponent, "GetMaxBurden", 0);
                g_miBagAddExtraBurden = g_IL2CPP->class_get_method_from_name(g_klassBagComponent, "AddExtraBurden", 1);
                g_miBagRemoveExtraBurden = g_IL2CPP->class_get_method_from_name(g_klassBagComponent, "RemoveExtraBurden", 1);
            }
            if (g_klassItemManager)
                g_miItemsRefreshTimeScale = g_IL2CPP->class_get_method_from_name(g_klassItemManager, "RefreshAllItemTimeScale", 0);
            g_modBatch3Inited = true;
            LOG_INFO("Mod SDK batch3 ready (Durability/Food/Bag)");
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModBatch3Init exception 0x%08X", GetExceptionCode());
            return false;
        }
    }

    static BagComponent_o* GetBagComponent()
    {
        return (BagComponent_o*)GetLeadingRoleComponent(g_klassBagComponent);
    }

    // 从 Furniture（BaseAgent）拿 FurnitureDurabilityComponent（mod AgentTools.GetAgentComponent 同款）
    static Il2CppObject* GetFurnitureDurability(BaseAgent_o* furniture)
    {
        if (!furniture || !furniture->agentComponentDictionary)
            return nullptr;
        return DictFindByKlass(furniture->agentComponentDictionary, g_klassFurnitureDurability);
    }

    // MaximumWithExtra：临时加 extra 看 GetMaxBurden（mod 同款）
    static int32_t MaximumWithExtra(Il2CppObject* bag, int32_t extra)
    {
        if (extra <= 0 || !g_miBagGetMaxBurden)
            return UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetMaxBurden, bag), 0);
        void* p[1] = { &extra };
        if (!InvokeOk(g_miBagAddExtraBurden, bag, p))
            return UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetMaxBurden, bag), 0);
        int32_t r = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetMaxBurden, bag), 0);
        InvokeOk(g_miBagRemoveExtraBurden, bag, p);
        return r;
    }

    // 二分找使 MaximumWithExtra(extra) >= target 的最小 extra（mod FindClosestExtraBurden 简化等价）
    static int32_t FindClosestExtraBurden(Il2CppObject* bag, int32_t target, int32_t naturalMaximum)
    {
        if (target <= naturalMaximum)
            return 0;
        int32_t lo = 0, hi = 100000000;
        while (MaximumWithExtra(bag, hi) < target && hi < 100000000)
            hi = (hi > 50000000) ? 100000000 : hi * 2;
        while (lo < hi)
        {
            int32_t mid = lo + (hi - lo) / 2;
            if (MaximumWithExtra(bag, mid) < target)
                lo = mid + 1;
            else
                hi = mid;
        }
        return lo;
    }
}

    // ---------- 物品柜/架容量增强辅助（懒虫 BagWhitelist/ConfigBagPatch 移植） ----------

    // ASCII 小写（避免依赖 <cctype>）
    static char AsciiLower(char ch)
    {
        return (ch >= 'A' && ch <= 'Z') ? (char)(ch + ('a' - 'A')) : ch;
    }

    // 不区分大小写子串匹配（ASCII 部分；中文 UTF-8 多字节不受影响）
    static bool StrIStr(const char* haystack, const char* needle)
    {
        if (!haystack || !needle || !needle[0])
            return false;
        size_t nl = strlen(needle);
        size_t hl = strlen(haystack);
        if (nl > hl)
            return false;
        for (size_t i = 0; i + nl <= hl; i++)
        {
            size_t j = 0;
            for (; j < nl; j++)
            {
                if (AsciiLower(haystack[i + j]) != AsciiLower(needle[j]))
                    break;
            }
            if (j == nl)
                return true;
        }
        return false;
    }

    // 容器名字识别：收纳架/置物架/冰箱（中英文都认；懒虫 IsShelfName/IsFridgeName 合并）
    static bool IsContainerName(const char* name)
    {
        if (!name || !name[0])
            return false;
        static const char* kKeywords[] = { "收纳架", "置物架", "冰箱", "shelf", "rack", "fridge", "refrigerator" };
        for (size_t i = 0; i < sizeof(kKeywords) / sizeof(kKeywords[0]); i++)
        {
            if (StrIStr(name, kKeywords[i]))
                return true;
        }
        return false;
    }

    // 对 Config_Bag 应用容器扩容（幂等：原始值记忆 + 按倍率重写；懒虫 ConfigBagPatch.ApplyTo 同款）
    static void ApplyContainerExpansion(Config_Bag_o* bag)
    {
        if (!bag)
            return;
        bool whitelisted = (bag->ID > 0) && (g_containerBagIds.count(bag->ID) != 0);
        if (!whitelisted)
        {
            char nameBuf[256] = { 0 };
            char nameBuf2[256] = { 0 };
            ILStringToUtf8(bag->Name, nameBuf, sizeof(nameBuf));
            ILStringToUtf8(bag->Name_Local, nameBuf2, sizeof(nameBuf2));
            if (!IsContainerName(nameBuf) && !IsContainerName(nameBuf2))
                return;
            if (bag->ID > 0)
                g_containerBagIds.insert(bag->ID);
        }
        // 行数 xN（Size[1]，列不动；懒虫只改行数）
        if (g_containerRowsMult > 1 && bag->Size)
        {
            void* items = *(void**)((uint8_t*)bag->Size + OFF_LIST_ITEMS);
            if (items)
            {
                int32_t* pRows = (int32_t*)((uint8_t*)items + OFF_ARRAY_DATA + sizeof(int32_t));
                if (g_origContainerRows.find(bag) == g_origContainerRows.end())
                    g_origContainerRows[bag] = *pRows;
                int32_t val = g_origContainerRows[bag] * g_containerRowsMult;
                if (val > 0 && val < 100000)
                    *pRows = val;
            }
        }
        // 负重 xN（Burden 字段直写）
        if (g_containerBurdenMult > 1)
        {
            if (g_origContainerBurden.find(bag) == g_origContainerBurden.end())
                g_origContainerBurden[bag] = bag->Burden;
            int64_t v = (int64_t)g_origContainerBurden[bag] * g_containerBurdenMult;
            bag->Burden = (v > INT32_MAX) ? INT32_MAX : (int32_t)v;
        }
    }

    // 原始值缓存上限保护（>2048 条清空重来，防读档对象指针累积；懒虫 C# 1024 同思路）
    static void CapContainerCache()
    {
        if (g_origContainerRows.size() + g_origContainerBurden.size() > 2048)
        {
            g_origContainerRows.clear();
            g_origContainerBurden.clear();
        }
    }

// ---------- 设施耐久 ----------
bool SLSDK_SetHomeDurability(int32_t slotTypeId, int32_t value, int32_t* updatedOut)
{
    if (updatedOut)
        *updatedOut = 0;
    if (!ModBatch3Init())
        return false;
    __try
    {
        Il2CppObject* am = GetWorldManager(OFF_BLW_AgentManager);
        if (!am || !g_miAgentGetHomeFurnitures)
            return false;
        void* p1[1] = { &slotTypeId };
        Il2CppObject* furnList = InvokeRet(g_miAgentGetHomeFurnitures, am, p1);
        if (!furnList)
            return false;
        size_t n = ListGetCount(furnList);
        int32_t updated = 0;
        for (size_t i = 0; i < n; i++)
        {
            BaseAgent_o* furn = (BaseAgent_o*)ListGetItem(furnList, i);
            Il2CppObject* dur = GetFurnitureDurability(furn);
            if (!dur || !g_miFurnSetMaxDurability || !g_miFurnSetCurDurability)
                continue;
            void* p2[1] = { &value };
            if (InvokeOk(g_miFurnSetMaxDurability, dur, p2) && InvokeOk(g_miFurnSetCurDurability, dur, p2))
                updated++;
        }
        if (updatedOut)
            *updatedOut = updated;
        return updated > 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_GetHomeDurabilitySummary(int32_t slotTypeId, int32_t* count, int32_t* minCurrent, int32_t* maxDurability)
{
    if (count)
        *count = 0;
    if (minCurrent)
        *minCurrent = -1;
    if (maxDurability)
        *maxDurability = -1;
    if (!ModBatch3Init())
        return false;
    __try
    {
        Il2CppObject* am = GetWorldManager(OFF_BLW_AgentManager);
        if (!am || !g_miAgentGetHomeFurnitures)
            return false;
        void* p1[1] = { &slotTypeId };
        Il2CppObject* furnList = InvokeRet(g_miAgentGetHomeFurnitures, am, p1);
        if (!furnList)
            return false;
        size_t n = ListGetCount(furnList);
        int32_t cnt = 0, mn = -1, mx = -1;
        for (size_t i = 0; i < n; i++)
        {
            BaseAgent_o* furn = (BaseAgent_o*)ListGetItem(furnList, i);
            Il2CppObject* dur = GetFurnitureDurability(furn);
            if (!dur || !g_miFurnGetCurDurability || !g_miFurnGetMaxDurability)
                continue;
            int32_t cur = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miFurnGetCurDurability, dur), -1);
            int32_t mx2 = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miFurnGetMaxDurability, dur), -1);
            if (mn < 0 || cur < mn)
                mn = cur;
            if (mx2 > mx)
                mx = mx2;
            cnt++;
        }
        if (count)
            *count = cnt;
        if (minCurrent)
            *minCurrent = mn;
        if (maxDurability)
            *maxDurability = mx;
        return cnt > 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// 设施耐久锁（任意槽位类型；对齐属性锁 SLSDK_ApplyAttrLocks：每帧保持；值相等跳过写）
void SLSDK_ApplyDurabilityLockSlot(int32_t slotTypeId, int32_t value)
{
    if (slotTypeId <= 0 || value <= 0)
        return;
    if (!ModBatch3Init())
        return;
    __try
    {
        Il2CppObject* am = GetWorldManager(OFF_BLW_AgentManager);
        if (!am || !g_miAgentGetHomeFurnitures)
            return;
        void* p1[1] = { &slotTypeId };
        Il2CppObject* furnList = InvokeRet(g_miAgentGetHomeFurnitures, am, p1);
        if (!furnList)
            return;
        size_t n = ListGetCount(furnList);
        for (size_t i = 0; i < n; i++)
        {
            BaseAgent_o* furn = (BaseAgent_o*)ListGetItem(furnList, i);
            Il2CppObject* dur = GetFurnitureDurability(furn);
            if (!dur)
                continue;
            bool needCur = g_miFurnGetCurDurability
                ? (UnboxInt32((Il2CppObject*)InvokeNoArg(g_miFurnGetCurDurability, dur), -1) != value)
                : true;
            bool needMax = g_miFurnGetMaxDurability
                ? (UnboxInt32((Il2CppObject*)InvokeNoArg(g_miFurnGetMaxDurability, dur), -1) != value)
                : true;
            if (needMax && g_miFurnSetMaxDurability)
            {
                void* p2[1] = { &value };
                InvokeOk(g_miFurnSetMaxDurability, dur, p2);
            }
            if (needCur && g_miFurnSetCurDurability)
            {
                void* p2[1] = { &value };
                InvokeOk(g_miFurnSetCurDurability, dur, p2);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// ---------- 无限食物开关 ----------
bool SLSDK_SetInfiniteFoodShelfLife(bool enabled, int32_t* affectedOut)
{
    if (affectedOut)
        *affectedOut = 0;
    if (!ModBatch3Init())
        return false;
    int32_t affected = 0;
    __try
    {
        // 遍历 items.Cache：Life > 0 的物品计数（mod SetInfiniteFoodShelfLife 同款）
        Il2CppObject* items = GetItemManager();
        if (items)
        {
            void* cache = *(void**)((uint8_t*)items + OFF_IM_Cache); // ItemManager.Cache
            if (cache)
            {
                Dictionary_o* d = (Dictionary_o*)cache;
                if (d->_entries && d->_count > 0 && d->_count <= 100000)
                {
                    uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
                    for (int32_t i = 0; i < d->_count; i++)
                    {
                        uint8_t* entry = entries + (size_t)i * 24;
                        void* val = *(void**)(entry + 16);
                        if (!val)
                            continue;
                        ItemData_o* item = (ItemData_o*)val;
                        Config_Item_o* cfg = GetConfigItem(item->ItemConfigId);
                        if (cfg && cfg->Life > 0)
                            affected++;
                    }
                }
            }
        }
        g_infiniteFoodShelfLife = enabled;
        if (!enabled && g_miItemsRefreshTimeScale && items)
            InvokeNoArg(g_miItemsRefreshTimeScale, items);
        if (affectedOut)
            *affectedOut = affected;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_InfiniteFoodEnabled()
{
    return g_infiniteFoodShelfLife;
}

// ---------- 背包尺寸 / 负重 ----------
bool SLSDK_GetBagInfo(SLBagInfo* out)
{
    if (!out)
        return false;
    memset(out, 0, sizeof(*out));
    if (!ModBatch3Init())
        return false;
    __try
    {
        BagComponent_o* bag = GetBagComponent();
        if (!bag)
            return false;
        if (g_miBagGetWeight)
            out->Weight = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetWeight, bag), -1);
        if (g_miBagGetMaxBurden)
            out->MaxBurden = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetMaxBurden, bag), -1);
        if (g_miBagGetConfigId && g_miConfigGetBag)
        {
            int32_t cfgId = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetConfigId, bag), -1);
            Il2CppObject* cm = GetConfigManager();
            if (cm && cfgId > 0)
            {
                Config_Bag_o* cfgBag = (Config_Bag_o*)InvokeIntArg(g_miConfigGetBag, cm, cfgId);
                if (cfgBag && cfgBag->Size)
                {
                    out->Columns = IntListGet(cfgBag->Size, 0, -1);
                    out->Rows = IntListGet(cfgBag->Size, 1, -1);
                }
            }
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_SetBagSize(int32_t columns, int32_t rows)
{
    if (!ModBatch3Init())
        return false;
    __try
    {
        if (columns < 1 || columns > 20 || rows < 1 || rows > 20)
            return false;
        BagComponent_o* bag = GetBagComponent();
        Il2CppObject* cm = GetConfigManager();
        if (!bag || !cm || !g_miBagGetConfigId || !g_miConfigGetBag)
            return false;
        int32_t cfgId = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetConfigId, bag), -1);
        if (cfgId <= 0)
            return false;
        Config_Bag_o* cfgBag = (Config_Bag_o*)InvokeIntArg(g_miConfigGetBag, cm, cfgId);
        if (!cfgBag || !cfgBag->Size)
            return false;
        // 首次设置时记忆原始尺寸（mod OriginalSizes 同款）
        if (g_bagSizeCols <= 0 || g_bagSizeRows <= 0)
        {
            g_bagSizeCols = IntListGet(cfgBag->Size, 0, columns);
            g_bagSizeRows = IntListGet(cfgBag->Size, 1, rows);
        }
        // 记录期望尺寸（mod SetRememberedSize 同款，hook GetOwnerBagSize 用）
        g_desiredBagCols = columns;
        g_desiredBagRows = rows;
        // 直接改 Config_Bag.Size（mod SetSizeCore 同款）
        void* sizeList = cfgBag->Size;
        void* items = *(void**)((uint8_t*)sizeList + OFF_LIST_ITEMS);
        if (!items)
            return false;
        *(int32_t*)((uint8_t*)items + OFF_ARRAY_DATA) = columns;
        *(int32_t*)((uint8_t*)items + OFF_ARRAY_DATA + sizeof(int32_t)) = rows;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_ResetBagSize()
{
    if (!ModBatch3Init() || g_bagSizeCols <= 0 || g_bagSizeRows <= 0)
        return false;
    g_desiredBagCols = 0;
    g_desiredBagRows = 0;
    return SLSDK_SetBagSize(g_bagSizeCols, g_bagSizeRows);
}

bool SLSDK_SetMaxBurden(int32_t target)
{
    if (!ModBatch3Init())
        return false;
    __try
    {
        BagComponent_o* bag = GetBagComponent();
        if (!bag || !g_miBagGetMaxBurden || !g_miBagAddExtraBurden || !g_miBagRemoveExtraBurden)
            return false;
        // 先移除上次补足（mod RemoveAppliedExtraBurden 同款）
        if (g_appliedExtraBurden > 0)
        {
            void* pr[1] = { &g_appliedExtraBurden };
            InvokeOk(g_miBagRemoveExtraBurden, bag, pr);
            g_appliedExtraBurden = 0;
        }
        int32_t natural = UnboxInt32((Il2CppObject*)InvokeNoArg(g_miBagGetMaxBurden, bag), -1);
        if (natural < 0)
            return false;
        if (target < natural)
            return false; // mod：不能小于原始最大负重
        int32_t extra = FindClosestExtraBurden((Il2CppObject*)bag, target, natural);
        if (extra > 0)
        {
            void* pa[1] = { &extra };
            if (!InvokeOk(g_miBagAddExtraBurden, bag, pa))
                return false;
            g_appliedExtraBurden = extra;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool SLSDK_ResetMaxBurden()
{
    if (!ModBatch3Init())
        return false;
    __try
    {
        BagComponent_o* bag = GetBagComponent();
        if (!bag || !g_miBagRemoveExtraBurden)
            return false;
        if (g_appliedExtraBurden > 0)
        {
            void* p[1] = { &g_appliedExtraBurden };
            InvokeOk(g_miBagRemoveExtraBurden, bag, p);
            g_appliedExtraBurden = 0;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

// ---------- 物品柜/架容量增强（懒虫增强版白名单扩容移植） ----------
// 收纳架/置物架/冰箱按名字自动进白名单（Get_Config_Bag hook 应用），行数xrowsMult、负重xburdenMult
bool SLSDK_SetContainerExpansion(int32_t rowsMult, int32_t burdenMult)
{
    if (rowsMult < 1 || rowsMult > 100 || burdenMult < 1 || burdenMult > 1000)
        return false;
    if (!ModBatch3Init())
        return false;
    g_containerRowsMult = rowsMult;
    g_containerBurdenMult = burdenMult;
    __try
    {
        // Reapply：遍历已记忆原始值的容器对象，按新倍率重写（懒虫 ConfigBagPatch.Reapply 同款）
        for (std::map<void*, int32_t>::iterator it = g_origContainerRows.begin(); it != g_origContainerRows.end(); ++it)
        {
            Config_Bag_o* bag = (Config_Bag_o*)it->first;
            if (!bag || !bag->Size)
                continue;
            void* items = *(void**)((uint8_t*)bag->Size + OFF_LIST_ITEMS);
            if (!items)
                continue;
            int32_t* pRows = (int32_t*)((uint8_t*)items + OFF_ARRAY_DATA + sizeof(int32_t));
            int32_t val = it->second * g_containerRowsMult;
            if (val > 0 && val < 100000)
                *pRows = val;
        }
        for (std::map<void*, int32_t>::iterator it = g_origContainerBurden.begin(); it != g_origContainerBurden.end(); ++it)
        {
            Config_Bag_o* bag = (Config_Bag_o*)it->first;
            if (!bag)
                continue;
            int64_t v = (int64_t)it->second * g_containerBurdenMult;
            bag->Burden = (v > INT32_MAX) ? INT32_MAX : (int32_t)v;
        }
        LOG_INFO("ContainerExpansion set: rows x%d burden x%d (whitelist=%zu)", rowsMult, burdenMult, g_containerBagIds.size());
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void SLSDK_ResetContainerExpansion()
{
    SLSDK_SetContainerExpansion(1, 1);
    __try
    {
        g_origContainerRows.clear();
        g_origContainerBurden.clear();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void SLSDK_GetContainerExpansion(int32_t* rowsMult, int32_t* burdenMult)
{
    if (rowsMult)
        *rowsMult = g_containerRowsMult;
    if (burdenMult)
        *burdenMult = g_containerBurdenMult;
}

// ============================================================
// 批次4：Detours hook 层（mod Harmony 补丁的原生版，用 HookManager.h 封装）
// 目标（dump.cs 签名确认）：
//   ItemManager.GetOwnerBagSize(long) -> Vector2Int      （背包尺寸，mod BagSizePatch）
//   ItemManager.CheckAndProcessRot(ItemData,Config_Item,int)（无限食物，mod InfiniteFoodRotPatch）
//   GameTimeManager.CostTime(float,bool)                  （冻结拦截，mod CostTimePatch）
//   CountDownTimer.CostTime(float,bool)                   （冻结拦截，mod CostTimeCountDownPatch）
//   CountUpTimer.CostTime(float,bool)                     （冻结拦截，mod CostTimeCountUpPatch）
//   GameTimeManager.Update(float,float)                   （冻结保持，mod TimeUpdatePatch）
//   Ac_Player_UpdateCostHour.SendAction(int) static       （扣时拦截，mod CostHourPatch）
// 安装：HookManager::HookFunction(方法指针, hook函数)；原实现：CALL_ORIGIN(hook函数, ...)
// ============================================================
#include "../Hook/HookManager.h"

namespace
{
    bool g_hooksInstalled = false;
    // 冻结拦截一次性日志（诊断用）
    bool g_logGtmBlocked = false;
    bool g_logCdBlocked = false;
    bool g_logCuBlocked = false;
    bool g_logSendBlocked = false;

    // 取 MethodInfo 的原生入口（MethodInfo.methodPointer 在 +0x00，所有 IL2CPP 版本标准）
    static void* GetMethodPtr(Il2CppClass* klass, const char* name, int argc)
    {
        if (!klass || !g_IL2CPP)
            return nullptr;
        MethodInfo* mi = g_IL2CPP->class_get_method_from_name(klass, name, argc);
        if (!mi)
            return nullptr;
        return *(void**)((uint8_t*)mi + OFF_MI_METHODPOINTER);
    }

    // ---------- hook 函数（原实现用 CALL_ORIGIN） ----------
    struct V2I
    {
        int32_t x;
        int32_t y;
    };

    // 背包尺寸（mod BagSizePatch Postfix：期望尺寸 < 原始时取大）
    static V2I Hook_GetOwnerBagSize(void* self, int64_t ownerId)
    {
        __try
        {
            V2I r = CALL_ORIGIN(Hook_GetOwnerBagSize, self, ownerId);
            if (g_desiredBagCols > 0 && g_desiredBagRows > 0 && ownerId == GetPlayerId())
            {
                if (r.x < g_desiredBagCols)
                    r.x = g_desiredBagCols;
                if (r.y < g_desiredBagRows)
                    r.y = g_desiredBagRows;
            }
            return r;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            V2I r = { 0, 0 };
            return r;
        }
    }

    // 无限食物：Life>0 的物品重置 StartTime 并跳过腐烂处理（mod InfiniteFoodRotPatch Prefix）
    static void Hook_CheckAndProcessRot(void* self, void* item, void* config, int32_t currentHour)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && item && config)
            {
                Config_Item_o* cfg = (Config_Item_o*)config;
                if (cfg->Life > 0)
                {
                    ((ItemData_o*)item)->StartTime = currentHour;
                    return;
                }
            }
            CALL_ORIGIN(Hook_CheckAndProcessRot, self, item, config, currentHour);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // 时间冻结拦截 ×3（mod CostTimePatch / CostTimeCountDownPatch / CostTimeCountUpPatch）
    static void Hook_GameTimeCostTime(void* self, float seconds, bool silent)
    {
        __try
        {
            if (g_frozenOverride)
            {
                if (!g_logGtmBlocked) { g_logGtmBlocked = true; LOG_INFO("[Freeze] GTM.CostTime blocked"); }
                return;
            }
            CALL_ORIGIN(Hook_GameTimeCostTime, self, seconds, silent);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    static void Hook_CountDownCostTime(void* self, float seconds, bool silent)
    {
        __try
        {
            if (g_frozenOverride)
            {
                if (!g_logCdBlocked) { g_logCdBlocked = true; LOG_INFO("[Freeze] CD.CostTime blocked"); }
                return;
            }
            CALL_ORIGIN(Hook_CountDownCostTime, self, seconds, silent);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    static void Hook_CountUpCostTime(void* self, float seconds, bool silent)
    {
        __try
        {
            if (g_frozenOverride)
            {
                if (!g_logCuBlocked) { g_logCuBlocked = true; LOG_INFO("[Freeze] CU.CostTime blocked"); }
                return;
            }
            CALL_ORIGIN(Hook_CountUpCostTime, self, seconds, silent);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // 冻结时强制 IsClockFrozen（mod TimeUpdatePatch Prefix）
    static void Hook_GameTimeUpdate(void* self, float dt, float udt)
    {
        __try
        {
            if (g_frozenOverride)
                ((GameTimeManager_o*)self)->_IsClockFrozen = true;
            CALL_ORIGIN(Hook_GameTimeUpdate, self, dt, udt);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // 扣小时动作拦截（mod CostHourPatch Prefix，static 方法）
    static void Hook_SendAction(int32_t costHour)
    {
        __try
        {
            if (g_frozenOverride)
            {
                if (!g_logSendBlocked) { g_logSendBlocked = true; LOG_INFO("[Freeze] SendAction blocked"); }
                return;
            }
            CALL_ORIGIN(Hook_SendAction, costHour);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // 物品柜/架容量增强（懒虫 ConfigBagPatch Postfix：Get_Config_Bag 返回时对白名单容器应用倍率）
    static void* Hook_GetConfigBag(void* self, int32_t bagId)
    {
        void* cfg = nullptr;
        __try
        {
            cfg = CALL_ORIGIN(Hook_GetConfigBag, self, bagId);
            if (cfg && (g_containerRowsMult > 1 || g_containerBurdenMult > 1))
            {
                ApplyContainerExpansion((Config_Bag_o*)cfg);
                CapContainerCache();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return cfg;
    }
}

// 安装全部 hook（幂等；panel 首次渲染时调用，无 __try，klass 均已验证非 null）
bool SLSDK_InstallHooks()
{
    if (g_hooksInstalled)
        return true;
    if (!ModBatch3Init())
        return false;
    Il2CppClass* klassCostHour = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "Ac_Player_UpdateCostHour");
    if (!klassCostHour)
    {
        LOG_ERROR("Hook class NULL: Ac_Player_UpdateCostHour");
        return false;
    }
    void* pGetOwnerBagSize = GetMethodPtr(g_klassItemManager, "GetOwnerBagSize", 1);
    void* pCheckAndProcessRot = GetMethodPtr(g_klassItemManager, "CheckAndProcessRot", 3);
    void* pGameTimeCostTime = GetMethodPtr(g_klassGameTimeManager, "CostTime", 2);
    void* pCountDownCostTime = GetMethodPtr(g_klassCountDownTimer, "CostTime", 2);
    void* pCountUpCostTime = GetMethodPtr(g_klassCountUpTimer, "CostTime", 2);
    void* pGameTimeUpdate = GetMethodPtr(g_klassGameTimeManager, "Update", 2);
    void* pSendAction = GetMethodPtr(klassCostHour, "SendAction", 1);
    void* pGetConfigBag = GetMethodPtr(g_klassConfigManager, "Get_Config_Bag", 1);
    if (!pGetOwnerBagSize || !pCheckAndProcessRot || !pGameTimeCostTime || !pCountDownCostTime ||
        !pCountUpCostTime || !pGameTimeUpdate || !pSendAction || !pGetConfigBag)
    {
        LOG_ERROR("Hook method resolve failed: bag=%p rot=%p gtmCost=%p cdCost=%p cuCost=%p upd=%p send=%p cfgBag=%p",
            pGetOwnerBagSize, pCheckAndProcessRot, pGameTimeCostTime, pCountDownCostTime,
            pCountUpCostTime, pGameTimeUpdate, pSendAction, pGetConfigBag);
        return false;
    }

	LOG_DEBUG("Installing hooks: bag=%p rot=%p gtmCost=%p cdCost=%p cuCost=%p upd=%p send=%p cfgBag=%p",
		pGetOwnerBagSize, pCheckAndProcessRot, pGameTimeCostTime, pCountDownCostTime,
		pCountUpCostTime, pGameTimeUpdate, pSendAction, pGetConfigBag);

    HookManager::HookFunction(pGetOwnerBagSize, Hook_GetOwnerBagSize);
    HookManager::HookFunction(pCheckAndProcessRot, Hook_CheckAndProcessRot);
    HookManager::HookFunction(pGameTimeCostTime, Hook_GameTimeCostTime);
    HookManager::HookFunction(pCountDownCostTime, Hook_CountDownCostTime);
    HookManager::HookFunction(pCountUpCostTime, Hook_CountUpCostTime);
    HookManager::HookFunction(pGameTimeUpdate, Hook_GameTimeUpdate);
    HookManager::HookFunction(pSendAction, Hook_SendAction);
    HookManager::HookFunction(pGetConfigBag, Hook_GetConfigBag);
   
    SLSDK_InstallFoodDisplayHooks();

    g_hooksInstalled = true;
    return g_hooksInstalled;
}




// ============================================================
// 批次4 扩展：无限食物显示层（mod InfiniteFoodShelfLifeTextPatch / ExpiredStatePatch / ShelfLifeDaysPatch）
// 目标（dump.cs）：6 个 Reducer_Web_* 的 static GetShelfLifeText(Config_Item,Data_Item,float)->string、
//   IsItemExpired(int,Data_Item,float)->bool；Reducer_Web_BackpackUI.GetShelfLifeDaysRaw(int,Data_Item,float)->float
// 方法可能定义在基类，用 FindMethodInHierarchy 沿继承链找；解析不到就跳过该 hook（mod 逐个 try 同款）
// ============================================================
namespace
{
    // "∞" 字符串（UTF-8：E2 88 9E），初始化时构造一次
    Il2CppString* g_infinityString = nullptr;
    void* g_infinityHandle = nullptr;  // GCHandle 固定 ∞ 字符串（防 GC 回收成悬垂）

    // 6 个 Reducer 类名（mod InfiniteFoodReduxTargets.Types 同款）
    static const char* g_reducerNames[6] = {
        "Reducer_Web_BackpackUI", "Reducer_Web_Brew", "Reducer_Web_Cooking",
        "Reducer_Web_RatCage", "Reducer_Web_ShopUI", "Reducer_Web_TradeUI",
    };

    // ---------- GetShelfLifeText hooks（static：无 self，返回 string=Il2CppString*） ----------
    static Il2CppString* Hook_GetShelfLifeText_BackpackUI(void* config, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && config && ((Config_Item_o*)config)->Life > 0)
                return g_infinityString;
            return CALL_ORIGIN(Hook_GetShelfLifeText_BackpackUI, config, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return g_infinityString;
        }
    }
    static Il2CppString* Hook_GetShelfLifeText_Brew(void* config, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && config && ((Config_Item_o*)config)->Life > 0)
                return g_infinityString;
            return CALL_ORIGIN(Hook_GetShelfLifeText_Brew, config, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return g_infinityString;
        }
    }
    static Il2CppString* Hook_GetShelfLifeText_Cooking(void* config, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && config && ((Config_Item_o*)config)->Life > 0)
                return g_infinityString;
            return CALL_ORIGIN(Hook_GetShelfLifeText_Cooking, config, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return g_infinityString;
        }
    }
    static Il2CppString* Hook_GetShelfLifeText_RatCage(void* config, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && config && ((Config_Item_o*)config)->Life > 0)
                return g_infinityString;
            return CALL_ORIGIN(Hook_GetShelfLifeText_RatCage, config, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return g_infinityString;
        }
    }
    static Il2CppString* Hook_GetShelfLifeText_ShopUI(void* config, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && config && ((Config_Item_o*)config)->Life > 0)
                return g_infinityString;
            return CALL_ORIGIN(Hook_GetShelfLifeText_ShopUI, config, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return g_infinityString;
        }
    }
    static Il2CppString* Hook_GetShelfLifeText_TradeUI(void* config, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && config && ((Config_Item_o*)config)->Life > 0)
                return g_infinityString;
            return CALL_ORIGIN(Hook_GetShelfLifeText_TradeUI, config, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return g_infinityString;
        }
    }

    // ---------- IsItemExpired hooks（static：返回 bool） ----------
    static bool Hook_IsItemExpired_BackpackUI(int32_t configLife, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife)
                return false;
            return CALL_ORIGIN(Hook_IsItemExpired_BackpackUI, configLife, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
    static bool Hook_IsItemExpired_Brew(int32_t configLife, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife)
                return false;
            return CALL_ORIGIN(Hook_IsItemExpired_Brew, configLife, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
    static bool Hook_IsItemExpired_Cooking(int32_t configLife, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife)
                return false;
            return CALL_ORIGIN(Hook_IsItemExpired_Cooking, configLife, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
    static bool Hook_IsItemExpired_RatCage(int32_t configLife, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife)
                return false;
            return CALL_ORIGIN(Hook_IsItemExpired_RatCage, configLife, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
    static bool Hook_IsItemExpired_ShopUI(int32_t configLife, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife)
                return false;
            return CALL_ORIGIN(Hook_IsItemExpired_ShopUI, configLife, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
    static bool Hook_IsItemExpired_TradeUI(int32_t configLife, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife)
                return false;
            return CALL_ORIGIN(Hook_IsItemExpired_TradeUI, configLife, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // ---------- GetShelfLifeDaysRaw（仅 BackpackUI，static：返回 float） ----------
    static float Hook_GetShelfLifeDaysRaw(int32_t configLife, void* item, float hours)
    {
        __try
        {
            if (g_infiniteFoodShelfLife && configLife > 0)
                return (float)configLife / 24.0f;
            return CALL_ORIGIN(Hook_GetShelfLifeDaysRaw, configLife, item, hours);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return (float)configLife / 24.0f;
        }
    }

    // 物品详情弹窗保质期（mod 无此 patch，C++ 补充；Reducer_Web_ItemDetailPopup.SetShelfLifeParts static）
    // 无限开启时把 valueText 换成 ∞，点开物品详情不再显示真实倒计时
    static void Hook_SetShelfLifeParts(void* state, void* format, void* valueText)
    {
        __try
        {
            if (g_infiniteFoodShelfLife)
                valueText = g_infinityString;
            CALL_ORIGIN(Hook_SetShelfLifeParts, state, format, valueText);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

// 安装无限食物显示层 hook（由 SLSDK_InstallHooks 调用；无 __try）
bool SLSDK_InstallFoodDisplayHooks()
{
    if (!g_hooksInstalled)
        return false; // 先装核心 hook
    if (!g_IL2CPP || !g_IL2CPP->string_new)
        return false;
    if (!g_infinityString)
    {
        g_infinityString = g_IL2CPP->string_new("\xE2\x88\x9E"); // "∞" UTF-8
        // GCHandle 固定：C++ 全局裸指针不在 GC root 里，不固定会被回收（打开背包时 GetShelfLifeText 返回悬垂 → 崩溃）
        if (g_infinityString && g_IL2CPP->gchandle_new)
            g_infinityHandle = g_IL2CPP->gchandle_new((Il2CppObject*)g_infinityString, false);
    }
    int32_t installed = 0;
    for (int32_t i = 0; i < 6; i++)
    {
        Il2CppClass* reducer = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", g_reducerNames[i]);
        if (!reducer)
            continue;
        // GetShelfLifeText
        MethodInfo* miText = FindMethodInHierarchy(reducer, "GetShelfLifeText", 3);
        if (miText)
        {
            void* p = *(void**)((uint8_t*)miText + OFF_MI_METHODPOINTER);
            switch (i)
            {
            case 0: HookManager::HookFunction(p, Hook_GetShelfLifeText_BackpackUI); break;
            case 1: HookManager::HookFunction(p, Hook_GetShelfLifeText_Brew); break;
            case 2: HookManager::HookFunction(p, Hook_GetShelfLifeText_Cooking); break;
            case 3: HookManager::HookFunction(p, Hook_GetShelfLifeText_RatCage); break;
            case 4: HookManager::HookFunction(p, Hook_GetShelfLifeText_ShopUI); break;
            case 5: HookManager::HookFunction(p, Hook_GetShelfLifeText_TradeUI); break;
            }
            installed++;
        }
        // IsItemExpired
        MethodInfo* miExpired = FindMethodInHierarchy(reducer, "IsItemExpired", 3);
        if (miExpired)
        {
            void* p = *(void**)((uint8_t*)miExpired + OFF_MI_METHODPOINTER);
            switch (i)
            {
            case 0: HookManager::HookFunction(p, Hook_IsItemExpired_BackpackUI); break;
            case 1: HookManager::HookFunction(p, Hook_IsItemExpired_Brew); break;
            case 2: HookManager::HookFunction(p, Hook_IsItemExpired_Cooking); break;
            case 3: HookManager::HookFunction(p, Hook_IsItemExpired_RatCage); break;
            case 4: HookManager::HookFunction(p, Hook_IsItemExpired_ShopUI); break;
            case 5: HookManager::HookFunction(p, Hook_IsItemExpired_TradeUI); break;
            }
            installed++;
        }
    }
    // GetShelfLifeDaysRaw（仅 BackpackUI）
    Il2CppClass* backpackUI = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "Reducer_Web_BackpackUI");
    if (backpackUI)
    {
        MethodInfo* miDays = FindMethodInHierarchy(backpackUI, "GetShelfLifeDaysRaw", 3);
        if (miDays)
        {
            void* p = *(void**)((uint8_t*)miDays + OFF_MI_METHODPOINTER);
            HookManager::HookFunction(p, Hook_GetShelfLifeDaysRaw);
            installed++;
        }
    }
    // 物品详情弹窗保质期显示（点开物品后的 ItemDetailPopup；SetShelfLifeParts 是唯一保质期文本入口）
    Il2CppClass* itemDetail = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.ReduxUI", "Reducer_Web_ItemDetailPopup");
    if (itemDetail)
    {
        MethodInfo* miParts = FindMethodInHierarchy(itemDetail, "SetShelfLifeParts", 3);
        if (miParts)
        {
            void* p = *(void**)((uint8_t*)miParts + OFF_MI_METHODPOINTER);
            HookManager::HookFunction(p, Hook_SetShelfLifeParts);
            installed++;
            LOG_INFO("[Food] ItemDetailPopup.SetShelfLifeParts hooked");
        }
    }
    LOG_INFO("SurvivalLog food display hooks installed: %d/14", installed);
    return installed > 0;
}


// ============================================================
// 批次6：dexter.sl（3DM mod）功能迁移 —— 倍率类（保留项）
// 对应 C# mod dexter.sl v1.5.3：ConfigActionPatch(动作速度) / CookingDurationPatch(烹饪时间) /
//   ConfigInstancePatch+ExposureDetour(暴露速率)
// 实现：Config_Action 配置对象直写（Get_Config_Action hook 返回时应用，不干预进行中动作字段）+
//       Detours hook（OnCookingStart/AddExposure）
// 全免更：类按名解析，字段偏移 ResolveFieldOffset 动态解析（offsets.h 兜底）
// Config_Action（动作配置；During 为动作时长秒数，ActionType==2 跳过缩放）
struct Config_Action_o : Il2CppObject
{
    int32_t ID;            // +0x10
    void* Name;            // +0x18
    void* Name_Local;      // +0x20
    int32_t ActionType;    // +0x40
    float During;          // +0x48
};

// ============================================================
namespace
{
    bool g_modBatch6Inited = false;
    bool g_batch6HooksInstalled = false;
    Il2CppClass* g_klassConfigAction = nullptr;
    Il2CppClass* g_klassFurniture = nullptr;
    Il2CppClass* g_klassCookingStartData = nullptr;

    // 动作速度倍率（mod ConfigActionPatch：During / N，ActionType==2 跳过）
    bool g_actionSpeedOn = false;
    int32_t g_actionSpeedMult = 1;
    std::map<void*, float> g_actionOrigDuring;

    // 烹饪时间倍率（mod CookingDurationPatch：OnCookingStart 参数 CookDuration / N）
    bool g_cookTimeOn = false;
    int32_t g_cookTimeMult = 1;

    // 暴露增长速率倍率（mod ConfigInstancePatch + ExposureDetour）
    bool g_exposureRateOn = false;
    float g_exposureRate = 1.0f;
    float g_origTimeExposure = -1.0f;
    float g_origMoveExposure = -1.0f;
    void* g_exposureOwner = nullptr;

    static bool ModBatch6Init()
    {
        if (g_modBatch6Inited)
            return true;
        if (!ModItemsInit() || !ModBatch2Init() || !ModBatch3Init())
            return false;
        __try
        {
            g_klassConfigAction = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "Config_Action");
            g_klassFurniture = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "Furniture");
            g_klassCookingStartData = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate", "CookingStartData");
            if (!g_klassConfigAction || !g_klassFurniture || !g_klassExploreManager || !g_klassCookingStartData)
            {
                LOG_ERROR("ModBatch6 class NULL: Action=%p Furniture=%p Explore=%p CookData=%p",
                    g_klassConfigAction, g_klassFurniture, g_klassExploreManager, g_klassCookingStartData);
                return false;
            }
            OFF_CM_ActionDict = ResolveFieldOffset(g_klassConfigManager, "_Config_Action_Dict", OFF_CM_ActionDict);
            OFF_CA_ActionType = ResolveFieldOffset(g_klassConfigAction, "<ActionType>k__BackingField", OFF_CA_ActionType);
            OFF_CA_During = ResolveFieldOffset(g_klassConfigAction, "<During>k__BackingField", OFF_CA_During);
            OFF_CD_CookDuration = ResolveFieldOffset(g_klassCookingStartData, "<CookDuration>k__BackingField", OFF_CD_CookDuration);
            g_modBatch6Inited = true;
            LOG_INFO("Mod SDK batch6 ready (dexter.sl 倍率功能)");
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModBatch6Init exception 0x%08X", GetExceptionCode());
            return false;
        }
    }



    // 单个动作应用倍率（mod ConfigActionPatch.ApplyTo：During / N，ActionType==2 跳过）
    static void ApplyActionSpeedToOne(Config_Action_o* cfg)
    {
        if (!cfg)
            return;
        uint8_t* p = (uint8_t*)cfg;
        if (*(int32_t*)(p + OFF_CA_ActionType) == 2) // mod：类型 2 跳过
            return;
        auto it = g_actionOrigDuring.find(cfg);
        if (it == g_actionOrigDuring.end())
        {
            if (g_actionOrigDuring.size() > 1024)
                g_actionOrigDuring.clear(); // 防异常累积
            g_actionOrigDuring[cfg] = *(float*)(p + OFF_CA_During);
            it = g_actionOrigDuring.find(cfg);
        }
        float orig = it->second;
        *(float*)(p + OFF_CA_During) = (g_actionSpeedMult > 0) ? (orig / (float)g_actionSpeedMult) : orig;
    }

}





// ---------- 动作速度倍率（mod ConfigActionPatch：During / N，ActionType==2 跳过） ----------
bool SLSDK_SetActionSpeedMultiplier(int32_t mult)
{
    if (!ModBatch6Init())
        return false;
    g_actionSpeedMult = mult < 1 ? 1 : mult;
    g_actionSpeedOn = true;
    SLSDK_ApplyActionSpeedMultiplier();
    return true;
}

void SLSDK_ResetActionSpeedMultiplier()
{
    if (!g_actionSpeedOn)
        return;
    if (!ModBatch6Init())
        return;
    __try
    {
        for (auto& kv : g_actionOrigDuring)
        {
            if (!kv.first)
                continue;
            *(float*)((uint8_t*)kv.first + OFF_CA_During) = kv.second;
        }
        g_actionOrigDuring.clear();
        g_actionSpeedOn = false;
        g_actionSpeedMult = 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void SLSDK_ApplyActionSpeedMultiplier()
{
    if (!g_actionSpeedOn || !ModBatch6Init())
        return;
    __try
    {
        // 仅手动调用（Set 时一次）：遍历全部动作配置应用倍率。
        // 注意：不能每帧调用！游戏进行中动作会把 Config_Action.During 当倒计时字段递减，
        // 每帧覆盖会把倒计时重置回原值导致动作卡死（dexter.sl 只在 Get_Config_Action 返回时应用一次）。
        // 新发起的动作由 Hook_GetConfigAction 自动应用。
        Il2CppObject* cm = GetConfigManager();
        if (!cm)
            return;
        void* dict = *(void**)((uint8_t*)cm + OFF_CM_ActionDict); // ConfigManager._Config_Action_Dict
        Dictionary_o* d = (Dictionary_o*)dict;
        if (!d || !d->_entries || d->_count <= 0 || d->_count > 100000)
            return;
        uint8_t* entries = (uint8_t*)d->_entries + OFF_ARRAY_DATA;
        int32_t countBase = d->_count;
        void* entriesBase = d->_entries;
        for (int32_t i = 0; i < d->_count; i++)
        {
            // 配置热重载防护：遍历中字典被替换/清空则放弃本次（防悬垂 entries）
            if ((i & 63) == 0 && (d->_entries != entriesBase || d->_count != countBase))
                break;
            uint8_t* entry = entries + (size_t)i * 24;
            void* val = *(void**)(entry + 16);
            if (!val)
                continue;
            ApplyActionSpeedToOne((Config_Action_o*)val);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// ---------- 烹饪时间倍率（mod CookingDurationPatch：OnCookingStart 参数 CookDuration / N，下限 1s） ----------
bool SLSDK_SetCookingTimeMultiplier(int32_t mult)
{
    if (!ModBatch6Init())
        return false;
    g_cookTimeMult = mult < 1 ? 1 : mult;
    g_cookTimeOn = true;
    return true;
}

void SLSDK_ResetCookingTimeMultiplier()
{
    g_cookTimeOn = false;
    g_cookTimeMult = 1;
}

// ---------- 暴露增长速率倍率（mod ConfigInstancePatch + ExposureDetour） ----------
bool SLSDK_SetExposureRate(float rate)
{
    if (!ModBatch6Init())
        return false;
    g_exposureRate = rate < 0.01f ? 0.01f : (rate > 1.0f ? 1.0f : rate);
    g_exposureRateOn = true;
    SLSDK_ApplyExposureRate();
    return true;
}

void SLSDK_ResetExposureRate()
{
    if (!g_exposureRateOn)
        return;
    if (!ModBatch6Init())
        return;
    __try
    {
        ExploreManager_o* ex = (ExploreManager_o*)GetWorldManager(OFF_BLW_ExploreManager);
        if (ex && g_exposureOwner == ex && g_origTimeExposure >= 0.0f)
        {
            ex->TimeExposure = g_origTimeExposure;
            ex->MoveExposure = g_origMoveExposure;
        }
        g_exposureOwner = nullptr;
        g_origTimeExposure = -1.0f;
        g_origMoveExposure = -1.0f;
        g_exposureRateOn = false;
        g_exposureRate = 1.0f;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void SLSDK_ApplyExposureRate()
{
    if (!g_exposureRateOn || !ModBatch6Init())
        return;
    __try
    {
        ExploreManager_o* ex = (ExploreManager_o*)GetWorldManager(OFF_BLW_ExploreManager);
        if (!ex)
            return;
        if (g_exposureOwner != ex)
        {
            g_exposureOwner = ex;
            g_origTimeExposure = ex->TimeExposure;
            g_origMoveExposure = ex->MoveExposure;
        }
        if (g_exposureRate >= 0.999f)
        {
            if (g_origTimeExposure >= 0.0f)
            {
                ex->TimeExposure = g_origTimeExposure;
                ex->MoveExposure = g_origMoveExposure;
            }
            return;
        }
        ex->TimeExposure = g_origTimeExposure * g_exposureRate;
        ex->MoveExposure = g_origMoveExposure * g_exposureRate;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// ---------- 批次6 hook（动作速度 + 烹饪时间 + 暴露事件） ----------
// 动作速度（mod ConfigActionPatch Postfix：Get_Config_Action 返回时应用一次，不干预进行中动作的字段）
static void* Hook_GetConfigAction(void* self, int32_t id)
{
    void* cfg = nullptr;
    __try
    {
        cfg = CALL_ORIGIN(Hook_GetConfigAction, self, id);
        if (cfg && g_actionSpeedOn && g_actionSpeedMult > 1)
            ApplyActionSpeedToOne((Config_Action_o*)cfg);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return cfg;
}

static void Hook_OnCookingStart(void* self, void* data)
{
    __try
    {
        if (g_cookTimeOn && g_cookTimeMult > 1 && data)
        {
            float* p = (float*)((uint8_t*)data + OFF_CD_CookDuration);
            float v = *p;
            float nv = v / (float)g_cookTimeMult;
            if (nv < 1.0f)
                nv = 1.0f;
            *p = nv;
        }
        CALL_ORIGIN(Hook_OnCookingStart, self, data);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

static void Hook_AddExposure(void* self, int32_t value)
{
    __try
    {
        if (g_exposureRateOn && g_exposureRate < 0.999f)
        {
            double scaled = (double)value * (double)g_exposureRate;
            value = (int32_t)(scaled >= 0.0 ? (int32_t)(scaled + 0.5) : (int32_t)(scaled - 0.5)); // AwayFromZero
        }
        CALL_ORIGIN(Hook_AddExposure, self, value);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

bool SLSDK_InstallBatch6Hooks()
{
    if (g_batch6HooksInstalled)
        return true;
    if (!ModBatch6Init())
        return false;
    void* pGetCfgAction = GetMethodPtr(g_klassConfigManager, "Get_Config_Action", 1);
    void* pCook = GetMethodPtr(g_klassFurniture, "OnCookingStart", 1);
    void* pExpo = GetMethodPtr(g_klassExploreManager, "AddExposure", 1);
    if (!pGetCfgAction || !pCook || !pExpo)
    {
        LOG_ERROR("Batch6 hook resolve failed: act=%p cook=%p expo=%p", pGetCfgAction, pCook, pExpo);
        return false;
    }
    HookManager::HookFunction(pGetCfgAction, Hook_GetConfigAction);
    HookManager::HookFunction(pCook, Hook_OnCookingStart);
    HookManager::HookFunction(pExpo, Hook_AddExposure);
    g_batch6HooksInstalled = true;
    LOG_INFO("SurvivalLog batch6 hooks installed (Get_Config_Action/OnCookingStart/AddExposure)");
    return true;
}


// ============================================================
// 批次8：电力倍率（mod DexterSL_Extra.PowerManagerPatch：发电量/储电倍率）
// 目标（dump.cs / monodump 确认，全在 GameCore.HotUpdate.Battle.Logic.PowerManager）：
//   InjectPower(float)                          -> 发电倍率（mod InjectPowerPatch.Prefix: amount *= gen）
//   CalculateGeneration(List<Furniture>) -> float（发电倍率 Postfix: result *= gen; _TotalGeneration=result）
//   CalculateCapacity(List<Furniture>)    -> float（储电倍率 Postfix: result *= cap; _TotalCapacity=result; _MaxPowerValue=clamp(result)）
// 字段偏移（PowerManager 实例）：
//   _CurPowerValue +0x34 / _MaxPowerValue +0x38 / _TotalCapacity +0x40 / _TotalGeneration +0x44
// 实例：BattleLogicWorld.Instance -> +OFF_BLW_PowerManager(0x188)
// ============================================================
namespace
{
    bool g_modPowerInited = false;
    bool g_powerHooksInstalled = false;
    Il2CppClass* g_klassPowerManager = nullptr;

    bool g_powerGenMultOn = false;
    float g_powerGenMult = 1.0f;
    bool g_powerCapMultOn = false;
    float g_powerCapMult = 1.0f;

    static bool ModPowerInit()
    {
        if (g_modPowerInited)
            return true;
        if (!ModItemsInit() || !ModBatch2Init())
            return false;
        __try
        {
            g_klassPowerManager = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "PowerManager");
            if (!g_klassPowerManager)
            {
                LOG_ERROR("ModPower class NULL: PowerManager=%p", g_klassPowerManager);
                return false;
            }
            OFF_PM_CurPowerValue   = ResolveFieldOffset(g_klassPowerManager, "<CurPowerValue>k__BackingField", OFF_PM_CurPowerValue);
            OFF_PM_MaxPowerValue   = ResolveFieldOffset(g_klassPowerManager, "<MaxPowerValue>k__BackingField", OFF_PM_MaxPowerValue);
            OFF_PM_TotalCapacity   = ResolveFieldOffset(g_klassPowerManager, "<TotalCapacity>k__BackingField", OFF_PM_TotalCapacity);
            OFF_PM_TotalGeneration = ResolveFieldOffset(g_klassPowerManager, "<TotalGeneration>k__BackingField", OFF_PM_TotalGeneration);
            g_modPowerInited = true;
            LOG_INFO("Mod SDK power ready (PowerManager 发电/储电倍率)");
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModPowerInit exception 0x%08X", GetExceptionCode());
            return false;
        }
    }

    // 发电倍率：InjectPower(amount) 的 amount *= gen（等价 mod InjectPowerPatch.Prefix）
    static void Hook_PM_InjectPower(void* self, float amount)
    {
        __try
        {
            if (g_powerGenMultOn && fabsf(g_powerGenMult - 1.0f) > 0.001f)
                amount *= g_powerGenMult;
            CALL_ORIGIN(Hook_PM_InjectPower, self, amount);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // 发电倍率：CalculateGeneration 返回值 *= gen（等价 mod CalculateGenerationPatch.Postfix）+ 写回 _TotalGeneration
    static float Hook_PM_CalcGeneration(void* self, void* electricalFurnitures)
    {
        __try
        {
            float r = CALL_ORIGIN(Hook_PM_CalcGeneration, self, electricalFurnitures);
            if (g_powerGenMultOn && fabsf(g_powerGenMult - 1.0f) > 0.001f)
            {
                r *= g_powerGenMult;
                *(float*)((uint8_t*)self + OFF_PM_TotalGeneration) = r;
            }
            return r;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0.0f;
        }
    }

    // 储电倍率：CalculateCapacity 返回值 *= cap（等价 mod CalculateCapacityPatch.Postfix）+ 写回 _TotalCapacity/_MaxPowerValue
    static float Hook_PM_CalcCapacity(void* self, void* electricalFurnitures)
    {
        __try
        {
            float r = CALL_ORIGIN(Hook_PM_CalcCapacity, self, electricalFurnitures);
            if (g_powerCapMultOn && fabsf(g_powerCapMult - 1.0f) > 0.001f)
            {
                r *= g_powerCapMult;
                *(float*)((uint8_t*)self + OFF_PM_TotalCapacity) = r;
                float cl = r < 0.0f ? 0.0f : (r > 2.1474836e9f ? 2.1474836e9f : r);
                *(int32_t*)((uint8_t*)self + OFF_PM_MaxPowerValue) = (int32_t)cl;
            }
            return r;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0.0f;
        }
    }
}

bool SLSDK_InstallPowerHooks()
{
    if (g_powerHooksInstalled)
        return true;
    if (!ModPowerInit())
        return false;
    void* pInject = GetMethodPtr(g_klassPowerManager, "InjectPower", 1);
    void* pGen    = GetMethodPtr(g_klassPowerManager, "CalculateGeneration", 1);
    void* pCap    = GetMethodPtr(g_klassPowerManager, "CalculateCapacity", 1);
    if (!pInject || !pGen || !pCap)
    {
        LOG_ERROR("Power hook resolve failed: inject=%p gen=%p cap=%p", pInject, pGen, pCap);
        return false;
    }
    HookManager::HookFunction(pInject, Hook_PM_InjectPower);
    HookManager::HookFunction(pGen,    Hook_PM_CalcGeneration);
    HookManager::HookFunction(pCap,    Hook_PM_CalcCapacity);
    g_powerHooksInstalled = true;
    LOG_INFO("SurvivalLog power hooks installed (InjectPower/CalculateGeneration/CalculateCapacity)");
    return true;
}

// 设置发电量/储电倍率（0.1~200；传 1 = 关闭该档）；成功后 hooks 已自动安装
bool SLSDK_SetPowerMultiplier(float genMult, float capMult)
{
    if (!SLSDK_InstallPowerHooks())
        return false;
    g_powerGenMult = genMult;
    g_powerCapMult = capMult;
    g_powerGenMultOn = (genMult >= 1.0f && genMult <= 200.0f && fabsf(genMult - 1.0f) > 0.001f);
    g_powerCapMultOn = (capMult >= 1.0f && capMult <= 200.0f && fabsf(capMult - 1.0f) > 0.001f);
    LOG_INFO("[电力] 发电倍率=%s x%.2f | 储电倍率=%s x%.2f",
        g_powerGenMultOn ? "ON" : "OFF", g_powerGenMult, g_powerCapMultOn ? "ON" : "OFF", g_powerCapMult);
    return true;
}

void SLSDK_ResetPowerMultiplier()
{
    g_powerGenMultOn = false;
    g_powerCapMultOn = false;
    g_powerGenMult = 1.0f;
    g_powerCapMult = 1.0f;
}



// ============================================================
// 批次9：丧尸击杀（尸潮）
// 目标：GameCore.HotUpdate.Battle.Logic.Zombie（:BaseAgent）
//   TakeHpDamage(int) -> 扣值伤害，传大值(999999)=秒杀（走正常死亡流程）
//   InstanceId +0x10 / CurrentHP +0x74 / MaxHP +0x78
// 枚举：BattleLogicWorld.Instance -> AgentManager._agentList（List<BaseAgent>），按类 == Zombie 过滤
// ============================================================
namespace
{
    bool g_modZombieInited = false;
    Il2CppClass* g_klassZombie = nullptr;
    MethodInfo* g_miZombieTakeHpDamage = nullptr;

    static bool ModZombieInit()
    {
        if (g_modZombieInited)
            return true;
        if (!ModItemsInit() || !ModBatch2Init())
            return false;
        __try
        {
            g_klassZombie = g_IL2CPP->class_from_name(g_hotUpdateImage, "GameCore.HotUpdate.Battle.Logic", "Zombie");
            if (!g_klassZombie)
            {
                LOG_ERROR("ModZombie class NULL: Zombie=%p", g_klassZombie);
                return false;
            }
            g_miZombieTakeHpDamage = g_IL2CPP->class_get_method_from_name(g_klassZombie, "TakeHpDamage", 1);
            OFF_ZOMBIE_CurrentHP = ResolveFieldOffset(g_klassZombie, "<CurrentHP>k__BackingField", OFF_ZOMBIE_CurrentHP);
            OFF_ZOMBIE_MaxHP     = ResolveFieldOffset(g_klassZombie, "<MaxHP>k__BackingField", OFF_ZOMBIE_MaxHP);
            if (!g_miZombieTakeHpDamage)
            {
                LOG_ERROR("ModZombie method NULL: TakeHpDamage");
                return false;
            }
            g_modZombieInited = true;
            LOG_INFO("Mod SDK zombie ready (Zombie 尸潮击杀)");
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            LOG_ERROR("ModZombieInit exception 0x%08X", GetExceptionCode());
            return false;
        }
    }

    // 从 AgentManager.agentList 收集全部 Zombie 实例
    static std::vector<void*> CollectZombies()
    {
        std::vector<void*> out;

            BattleLogicWorld_o* world = (BattleLogicWorld_o*)GetSingletonInstance(g_klassBattleLogicWorld);
            if (!world || !world->_AgentManager)
                return out;
            void* agentList = world->_AgentManager->_agentList;
            size_t count = ListGetCount(agentList);
            if (count > 512) count = 512;
            for (size_t i = 0; i < count; ++i)
            {
                void* agent = ListGetItem(agentList, i);
                if (!agent)
                    continue;
                if (g_IL2CPP->object_get_class((Il2CppObject*)agent) != g_klassZombie)
                    continue;
                out.push_back(agent);
            }

        return out;
    }
}

int32_t SLSDK_GetZombies(SLZombieView* outItems, int32_t maxItems)
{
    if (!ModZombieInit())
        return -1;
    std::vector<void*> zs = CollectZombies();
    int32_t total = (int32_t)zs.size();
    if (outItems && maxItems > 0)
    {
        int32_t n = total < maxItems ? total : maxItems;
        for (int32_t i = 0; i < n; ++i)
        {
            uint8_t* z = (uint8_t*)zs[i];
            outItems[i].InstanceId = *(int64_t*)(z + OFF_ZOMBIE_InstanceId);
            outItems[i].CurrentHP  = *(int32_t*)(z + OFF_ZOMBIE_CurrentHP);
            outItems[i].MaxHP      = *(int32_t*)(z + OFF_ZOMBIE_MaxHP);
        }
    }
    return total;
}

bool SLSDK_KillZombie(int64_t instanceId)
{
    if (!ModZombieInit())
        return false;
    if (g_domain && g_IL2CPP && g_IL2CPP->thread_attach)
        g_IL2CPP->thread_attach(g_domain);
    std::vector<void*> zs = CollectZombies();
    for (size_t i = 0; i < zs.size(); ++i)
    {
        uint8_t* z = (uint8_t*)zs[i];
        if (*(int64_t*)(z + OFF_ZOMBIE_InstanceId) == instanceId)
            return InvokeIntArgOk(g_miZombieTakeHpDamage, z, 999999);
    }
    return false;
}

int32_t SLSDK_KillAllZombies()
{
    if (!ModZombieInit())
        return -1;
    if (g_domain && g_IL2CPP && g_IL2CPP->thread_attach)
        g_IL2CPP->thread_attach(g_domain);
    std::vector<void*> zs = CollectZombies();
    int32_t killed = 0;
    for (size_t i = 0; i < zs.size(); ++i)
    {
        if (InvokeIntArgOk(g_miZombieTakeHpDamage, zs[i], 999999))
            ++killed;
    }
    return killed;
}


// ---------- 渲染线程 attach（幂等）：未 attach 线程在游戏 GC（stop-the-world）期间调托管 API 会偶发崩溃 ----------
void SLSDK_EnsureThreadAttached()
{
    if (g_domain && g_IL2CPP && g_IL2CPP->thread_attach)
        g_IL2CPP->thread_attach(g_domain);
}
