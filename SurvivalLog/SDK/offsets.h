#pragma once
// ============================================================
// offsets.h - SurvivalLog 全部字段偏移集中表
//
// 偏移来源（当前游戏版本 dump）：
//   SDK_dump\SurvivalLog_PC\il2cpp.h      （Il2CppDumper 生成的结构体布局）
//   SDK_dump\monodump_HotUpdate.txt       （字段名 @ 偏移，交叉验证）
//
// 使用约定（全免更）：
//   1. 代码里不写裸偏移（如 +0x128），统一引用这里的 OFF_* 变量
//   2. 这些是运行时变量：Init 时 ResolveFieldOffset() 按字段名动态解析并
//      自动填充真实值（SurvivalLogSDK.cpp 的 ResolveAllOffsets / ModItemsInit /
//      ModBatch2Init 段4），游戏更新后完全不用手动改这里
//   3. 默认值只是解析前的兜底（当前版本 dump 值）；解析失败才用到
//   4. 想核对当前实际生效值，看日志或调试时读变量即可
// ============================================================
#include <cstddef>

// ---------- IL2CPP 运行时通用布局（版本间基本稳定，一般不用改） ----------
inline size_t OFF_OBJ_HEADER = 0x10;        // Il2CppObject 对象头大小（所有托管对象首字段偏移）
inline size_t OFF_LIST_ITEMS = 0x10;        // List<T>._items（数组引用）
inline size_t OFF_LIST_SIZE = 0x18;         // List<T>._size（=Count）
inline size_t OFF_DICT_ENTRIES = 0x18;      // Dictionary<TKey,TValue>._entries（数组引用）
inline size_t OFF_ARRAY_DATA = 0x20;        // 数组数据区起点（GC 数组对象头 0x20；元素 i = +0x20 + i*elemSize）
inline size_t OFF_BOXED_VALUE = 0x10;       // 装箱对象（boxed）值区起点（值类型 runtime_invoke 返回值）
inline size_t OFF_STRING_LENGTH = 0x10;     // Il2CppString.length（int32）
inline size_t OFF_STRING_CHARS = 0x18;      // Il2CppString.chars（UTF-16 数据）
inline size_t OFF_RP_CURRENTVALUE = 0x20;   // R3.ReactiveProperty<T>.currentValue
inline size_t OFF_MI_METHODPOINTER = 0x00;  // MethodInfo.methodPointer（原生入口，所有 IL2CPP 版本标准）

// ---------- Attr（单个属性，BaseValue 存储 x1000，如 20000 = 20.000） ----------
inline size_t OFF_ATTR_BaseValue = 0x10;    // Attr.BaseValue
inline size_t OFF_ATTR_Max = 0x1C;          // Attr.Max

// ---------- AttributeComponent（玩家属性组件） ----------
inline size_t OFF_AC_AttrDict = 0x48;       // AttributeComponent.AttrDict（Dictionary<AttrName, Attr>）

// ---------- UserComponent（玩家用户数据，金币真源） ----------
inline size_t OFF_UC_Money = 0x50;          // UserComponent.Money

// ---------- BaseAgent ----------
inline size_t OFF_BA_agentComponentDictionary = 0x48; // BaseAgent.agentComponentDictionary（Dictionary<Type, BaseEntity>）

// ---------- AgentManager ----------
inline size_t OFF_AM_agentList = 0x30;      // AgentManager._agentList（List<BaseAgent>）

// ---------- BattleLogicWorld（游戏主世界单例，管理器都挂在这） ----------
inline size_t OFF_BLW_AgentManager = 0x40;              // BattleLogicWorld._AgentManager
inline size_t OFF_BLW_GameTimeManager = 0x80;           // BattleLogicWorld._GameTimeManager
inline size_t OFF_BLW_NeighborRescue = 0xC0;            // <_NeighborRescueManager>k__BackingField（邻居关系）
inline size_t OFF_BLW_ProficiencyManager = 0x128;       // <_ProficiencyManager>k__BackingField（熟练度）
inline size_t OFF_BLW_SurvivalResultsManager = 0x130;   // <_SurvivalResultsManager>k__BackingField（生存点）
inline size_t OFF_BLW_ExploreManager = 0x1C8;           // <_ExploreManager>k__BackingField（探索/暴露度）
inline size_t OFF_BLW_AchievementManager = 0x1E0;       // <_AchievementManager>k__BackingField（成就）
inline size_t OFF_BLW_CodexManager = 0x298;             // <_CodexManager>k__BackingField（图鉴）

// ---------- GameTimeManager ----------
inline size_t OFF_GTM_Timer = 0x38;         // GameTimeManager._Timer（ITimer = CountDownTimer）
inline size_t OFF_GTM_WorldStateType = 0x64; // GameTimeManager._WorldStateType（1=灾变前 2=灾变后）

// ---------- CountDownTimer ----------
inline size_t OFF_CDT_RemainTime = 0x10;    // CountDownTimer._RemainTime（x5 = 显示秒）

// ---------- State_Data_Player（Redux 数据层） ----------
inline size_t OFF_SDP_Gold = 0x2C;          // State_Data_Player._Gold（显示层）
inline size_t OFF_SDP_WorldState = 0x28;    // State_Data_Player._WorldState

// ---------- State_Web_CoreUI0 ----------
inline size_t OFF_SWC_DisplaySeconds = 0x38;      // _DisplaySeconds（ReactiveProperty<float>）
inline size_t OFF_SWC_DecreasePerSecond = 0x40;   // _DecreasePerSecond（ReactiveProperty<int>）

// ---------- State_Web_BuildShop（建造商店） ----------
inline size_t OFF_SWB_Gold = 0x30;          // <Gold>（ReactiveProperty<int> 商店显示金钱）
inline size_t OFF_SWB_PackageCapacity = 0x38; // <PackageCapacity>（ReactiveProperty<int> 待安装包裹）
inline size_t OFF_SWB_Products = 0x58;      // <Products>（ObservableList<Data_Web_BuildShop_Product>）

// ---------- Data_Web_BuildShop_Product ----------
inline size_t OFF_PRODUCT_SlotCurrent = 0x40; // <SlotCurrent>（ReactiveProperty<int> 库存数量）

// ---------- ShopComponent（逻辑层商店组件） ----------
inline size_t OFF_SC_ShopCache = 0x48;      // ShopComponent.ShopCache（List<ShopItem>）

// ---------- ShopItem / Data_ShopItem ----------
inline size_t OFF_SI_ItemCount = 0x14;      // ItemCount（库存，0=售罄）

// ---------- State_Data_Shop（Redux 商店数据层） ----------
inline size_t OFF_DATASHOP_ShopCache = 0x38; // <ShopCache>（List<Data_ShopItem>）

// ---------- State_Web_ShopUI（商店 UI 状态层） ----------
inline size_t OFF_SHOPUI_ShopCache = 0x70;  // <ShopCache>（List<Data_ShopItem>）

// ---------- State_Web_FurnitureDetail（家具详情 UI） ----------
inline size_t OFF_FD_CanBuy = 0x138;        // <CanBuy>（ReactiveProperty<bool>）
inline size_t OFF_FD_SoldOut = 0x148;       // <SoldOut>（ReactiveProperty<bool> 售罄显示）

// ---------- ReduxUISystem ----------
inline size_t OFF_RUI_reduxStoreLayer = 0x38; // reduxStoreLayer（ReduxStoreLayer*）
inline size_t OFF_RUI_logicAdapter = 0x50;    // _logicAdapter（LogicAdapter*）

// ---------- ReduxStoreLayer ----------
inline size_t OFF_RSL_dataTree = 0x10;      // dataTree
inline size_t OFF_RSL_cacheTree = 0x28;     // cacheTree
inline size_t OFF_RSL_stateTree = 0x40;     // stateTree（Dictionary<Type, BaseReduxState>）

// ---------- LogicAdapter（ReduxUI 逻辑适配器） ----------
inline size_t OFF_LA_currentWorldState = 0x38; // _currentWorldState（灾变阶段缓存）

// ---------- ConfigManager（配置字典） ----------
inline size_t OFF_CM_ItemDict = 0x1F8;      // ConfigManager._Config_Item_Dict（Dictionary<int, Config_Item>）
inline size_t OFF_CM_BuffDict = 0x68;       // ConfigManager._Config_Buff_Dict（Dictionary<int, Config_Buff>）
inline size_t OFF_CM_DailyRandomDict = 0xD0; // ConfigManager._Config_DailyRandom_Dict（Dictionary<int, Config_DailyRandom>）
inline size_t OFF_CM_RandomGroupDict = 0x288; // ConfigManager._Config_RandomGroup_Dict（Dictionary<int, Config_RandomGroup>）
inline size_t OFF_CM_TalentDict = 0x2F0;      // ConfigManager._Config_Talent_Dict（Dictionary<int, Config_Talent>）

// ---------- ItemManager ----------
inline size_t OFF_IM_Cache = 0x30;          // <Cache>k__BackingField（Dictionary<long, ItemData> 全部物品）

// ---------- BattleShowWorld（表现层世界） ----------
inline size_t OFF_BSW_CameraManager = 0x38; // <_CameraManager>k__BackingField（CameraManager*）
inline size_t OFF_BSW_HotKeyManager = 0x58; // <_HotKeyManager>k__BackingField（HotKeyManager*）

// ---------- CameraManager ----------
inline size_t OFF_CAM_isKeyboardMoveBlocked = 0x176; // CameraManager.isKeyboardMoveBlocked（bool 移动锁定）
