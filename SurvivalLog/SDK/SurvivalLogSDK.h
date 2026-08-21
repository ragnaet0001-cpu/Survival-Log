#pragma once
// ============================================================
// SurvivalLogSDK.h - SurvivalLog 游戏 SDK（规范 il2cpp 写法）
//
// 结构体布局来自 Il2CppDumper 生成的 SDK：
//   D:\Vsyuanma\SurvivalLog\SDK_dump\SurvivalLog_PC\il2cpp.h
//
// 字段偏移集中表：offsets.h（本文件结构体布局与其保持一致）
//
// 实例获取链（全部通过 il2cpp 运行时 API 按函数名解析）：
//   BattleLogicWorld.Instance (BaseSingleton.get_Instance)
//     -> _AgentManager(+0x40) -> GetLeadingRole() -> LeadingRole
//        -> agentComponentDictionary(+0x48) 找 AttributeComponent
//           -> AttrDict(+0x48) 按 AttrName 找 Attr -> BaseValue(+0x10)
//     -> _GameTimeManager(+0x80) -> _Timer(+0x30) -> CountDownTimer
//        -> _RemainTime(+0x10)   (x5 = 显示秒)
//   ReduxUISystem.Instance (BaseSingleton.get_Instance)
//     -> reduxStoreLayer(+0x38) -> stateTree(+0x40) 按 klass 找
//        -> State_Data_Player    _Gold(+0x2C)
//        -> State_Web_CoreUI0    _DisplaySeconds(+0x38, ReactiveProperty<float>)
//                                _DecreasePerSecond(+0x40, ReactiveProperty<int>)
// ============================================================
#include "il2cpp-api.h"

// ---------- 游戏结构体（字段偏移与 il2cpp.h 一致） ----------

// Attr: 单个属性（BaseValue 存储 x1000，如 20000 = 20.000）
struct Attr_o : Il2CppObject
{
    int32_t BaseValue;            // +0x10
    int32_t StrengtheningValue;   // +0x14
    int32_t ConditionValue;       // +0x18
    int32_t Max;                  // +0x1C
    int32_t Min;                  // +0x20
    bool isSingle;                // +0x24
};

struct BaseEntity_o : Il2CppObject
{
    int64_t InstanceId;       // +0x10
    int32_t InstanceIdType;   // +0x18
    void* ClassType;          // +0x20
    bool IsEventRegisterOpen; // +0x28
};

struct BaseComponent_o : BaseEntity_o
{
    int64_t ownerInstanceId; // +0x30
    int64_t showInstanceId;  // +0x38
};

struct AttributeComponent_o : BaseComponent_o
{
    bool IsNeedSyncAttr;          // +0x40
    void* AttrDict;               // +0x48  Dictionary<AttrName, Attr>
};

// UserComponent：玩家用户数据组件（Money 金币真源）
struct UserComponent_o : BaseComponent_o
{
    void* name;               // +0x40
    void* config;             // +0x48
    int32_t Money;            // +0x50  金币真源
    int32_t MapConfigId;      // +0x54
    int32_t MapConfigId_Home; // +0x58
    int32_t NextJumpMapId;    // +0x5C
    int32_t NextJumpCostTimeOverride; // +0x60
};

// ShopItem：商店商品（ItemCount 库存，0=售罄）
struct ShopItem_o : Il2CppObject
{
    int32_t ItemConfigId; // +0x10
    int32_t ItemCount;    // +0x14
    float FreshDecay;     // +0x18
    float Discount;       // +0x1C
};

// ShopComponent：商店组件（挂在商店家具实体上，ShopCache 是逻辑层库存）
struct ShopComponent_o : BaseComponent_o
{
    int32_t shopConfigId; // +0x40
    void* ShopCache;      // +0x48  List<ShopItem>
    void* shopConfigName; // +0x50
};

// Data_ShopItem：Redux 层的商品数据
struct Data_ShopItem_o : Il2CppObject
{
    int32_t ItemConfigId; // +0x10
    int32_t ItemCount;    // +0x14
    float FreshDecay;     // +0x18
    float Discount;       // +0x1C
};

struct BaseAgent_o : BaseEntity_o
{
    int32_t AgentConfigId;            // +0x30
    int64_t ShowInstanceId;           // +0x38
    bool IsDead;                      // +0x40
    void* agentComponentDictionary;   // +0x48  Dictionary<Type, BaseEntity>
    void* _attrRecoveryComponent;     // +0x50
    void* _transformComponent;        // +0x58
    void* _poorAppetiteComponent;     // +0x60
    void* _bagComponent;              // +0x68
};

struct LeadingRole_o : BaseAgent_o
{
    float _envAreaCheckTimer; // +0x70
    int32_t _lastEnvAreaId;   // +0x74
};

struct AgentManager_o : BaseEntity_o
{
    void* _agentList;         // +0x30  List<BaseAgent>
    void* _agentDictionary;   // +0x38  Dictionary<long, BaseAgent>
    int64_t _LeadingRoleId;   // +0x40
    void* _homeElecCache;     // +0x48
    bool _homeElecDirty;      // +0x50
    bool hasReportedDuplicateAgentSave; // +0x51
};

struct GameTimeManager_o : BaseEntity_o
{
    bool _IsGameStart;                  // +0x30
    void* _Timer;                       // +0x38  ITimer = CountDownTimer
    float _BattleTimeScale;             // +0x40
    bool _IsClockFrozen;                // +0x44
    float _TempSpeedUpBeforeScale;      // +0x48
    int32_t _PendingSpeedUpTriggerTime; // +0x4C
    int32_t _PendingSpeedUpType;        // +0x50
    int32_t _HistoryMaxDay;             // +0x54
    int32_t _ChapterId;                 // +0x58
    int32_t _ChapterSpeedUp1;           // +0x5C
    int32_t _ChapterSpeedUp2;           // +0x60
    int32_t _WorldStateType;            // +0x64
    float _TimeSyncTimer;               // +0x68
    bool _HalfInitWarned;               // +0x6C
    bool _pendingReopenDaySettlement;   // +0x6D
    bool _IsSlowGearActive;             // +0x6E
    float _IdleSlowBeforeScale;         // +0x70
    float _IdleSlowElapsed;             // +0x74
    float _IdleSlowCheckTimer;          // +0x78
};

struct CountDownTimer_o : Il2CppObject
{
    float _RemainTime;                 // +0x10  (x5 = 显示秒)
    float _TotalTime;                  // +0x14
    bool _IsCompleted;                 // +0x18
    float _ConsumedTime;               // +0x1C
    int32_t _LastHour;                 // +0x20
    int32_t _ExtraCountDownTimeConfig; // +0x24
    bool _HasTriggeredTwoHour;         // +0x28
    bool _HasTriggeredOneHour;         // +0x29
    bool _HasTriggeredDisasterImminent; // +0x2A
    int32_t _StartTime;                // +0x2C
};

// BaseSingleton<T> 实例字段（静态 _Instance 通过 get_Instance 方法拿，不用结构体）
struct BaseSingleton_Fields_o : Il2CppObject
{
    int64_t InstanceId; // +0x10
};

struct BattleLogicWorld_o : BaseSingleton_Fields_o
{
    bool _IsUpdate;                   // +0x18
    bool _IsPause;                    // +0x19
    void* _lastPauseStack;            // +0x20
    float _pausedElapsed;             // +0x28
    float _pauseWatchdogNextLogAt;    // +0x2C
    int32_t _userUIPauseHolds;        // +0x30
    bool _disableResponseActive;      // +0x34
    bool _IsPlanMode;                 // +0x35
    int32_t _CurrentViewFloorId;      // +0x38
    int32_t _GameModeKey;             // +0x3C
    AgentManager_o* _AgentManager;    // +0x40
    void* _FsmManager;                // +0x48
    void* _BTEffectManager;           // +0x50
    void* _DirectorManager;           // +0x58
    void* _RandomManager;             // +0x60
    void* _BtThinkingManager;         // +0x68
    void* _ActionManager;             // +0x70
    void* _ItemManager;               // +0x78
    GameTimeManager_o* _GameTimeManager; // +0x80
};

struct BaseReduxState_o : Il2CppObject
{
    void* _StateType;      // +0x10
    bool _rpMaybeDisposed; // +0x18
};

// State_Data_Shop：商店 Redux 数据层（依赖 BaseReduxState_o，必须在其后）
struct State_Data_Shop_o : BaseReduxState_o
{
    int64_t _BuyerId;      // +0x20
    int64_t _SellerId;     // +0x28
    int32_t _ShopConfigId; // +0x30
    void* _ShopCache;      // +0x38  List<Data_ShopItem>
};

// State_Web_ShopUI：商店 UI 状态层
struct State_Web_ShopUI_o : BaseReduxState_o
{
    int64_t _BuyerId;          // +0x20
    int64_t _SellerId;         // +0x28
    int32_t _ShopConfigId;     // +0x30
    int32_t _BagCols;          // +0x34
    int32_t _BagRows;          // +0x38
    int64_t _TrunkOwnerId;     // +0x40
    int32_t _TrunkCols;        // +0x48
    int32_t _TrunkRows;        // +0x4C
    int32_t _CurTab;           // +0x50
    void* _CurTabRP;           // +0x58
    void* _ItemDataList;       // +0x60
    void* _TrunkItemDataList;  // +0x68
    void* _ShopCache;          // +0x70  List<Data_ShopItem>
};

// LogicAdapter：ReduxUI 逻辑适配器（_currentWorldState 灾变阶段缓存）
struct LogicAdapter_o : BaseEntity_o
{
    int32_t _currentDay;             // +0x30
    float _battleTimeScaleCache;     // +0x34
    int32_t _currentWorldState;      // +0x38
};

// State_Web_FurnitureDetail：家具详情 UI（CanBuy/SoldOut 售罄显示）
struct State_Web_FurnitureDetail_o : BaseReduxState_o
{
    int32_t ConfigId;            // +0x20
    int64_t ShopInstanceId;      // +0x28
    void* FurnitureType;         // +0x30  RP<string>
    void* FurnitureConfigId;     // +0x38  RP<int>
    void* Name;                  // +0x40
    void* Icon;                  // +0x48
    void* Price;                 // +0x50
    void* SlotName;              // +0x58
    void* InstallTime;           // +0x60
    void* Description;           // +0x68
    void* RestoreCoeff;          // +0x70  RP<float>
    void* CapacityW;             // +0x78
    void* CapacityH;             // +0x80
    void* WeightMax;             // +0x88
    void* BatteryCapacity;       // +0x90
    void* CookType;              // +0x98
    void* MaxIngredient;         // +0xA0
    void* MaxSeasoning;          // +0xA8
    void* FuelSlotCount;         // +0xB0
    void* FuelConsume;           // +0xB8
    void* PowerConsume;          // +0xC0
    void* CookSpeed;             // +0xC8
    void* QualityBonus;          // +0xD0
    void* GenType;               // +0xD8
    void* ContinuousPower;       // +0xE0
    void* OneshotPower;          // +0xE8
    void* FacilitySpace;         // +0xF0
    void* LightCoeff;            // +0xF8
    void* TempCoeff;             // +0x100
    void* PestCoeff;             // +0x108
    void* WeedCoeff;             // +0x110
    void* GrowthAccel;           // +0x118
    void* DurabilityMax;         // +0x120
    void* DefReduceCoeff;        // +0x128
    void* FrostCoeff;            // +0x130
    void* CanBuy;                // +0x138  RP<bool>
    void* DemoTwoBlocked;        // +0x140  RP<bool>
    void* SoldOut;               // +0x148  RP<bool>
};

// Data_Web_BuildShop_Product：建造商店商品（SlotCurrent 库存）
struct Data_Web_BuildShop_Product_o : Il2CppObject
{
    int32_t Index;           // +0x10
    void* ConfigId;          // +0x18  RP<int>
    void* Name;              // +0x20  RP<string>
    void* Price;             // +0x28  RP<int>
    void* Icon;              // +0x30
    void* SizeName;          // +0x38
    void* SlotCurrent;       // +0x40  RP<int> 库存数量
    void* SlotMax;           // +0x48  RP<int>
    void* ShopPage;          // +0x50  RP<int>
    void* DemoTwoBlocked;    // +0x58  RP<bool>
};

// State_Web_BuildShop：当前打开的建造商店
struct State_Web_BuildShop_o : BaseReduxState_o
{
    int64_t ShopInstanceId;   // +0x20
    float FurnitureDiscountRatio; // +0x28
    float ShopDiscountRate;   // +0x2C
    void* Gold;               // +0x30  RP<int>
    void* PackageCapacity;    // +0x38
    void* PackageMaxCapacity; // +0x40
    void* ActiveTabId;        // +0x48
    void* Slots;              // +0x50  ObservableList
    void* Products;           // +0x58  ObservableList<Data_Web_BuildShop_Product>
    void* Tabs;               // +0x60
    void* Packages;           // +0x68
};

struct State_Data_Player_o : BaseReduxState_o
{
    int64_t _PlayerId;          // +0x20
    int32_t _WorldState;        // +0x28
    int32_t _Gold;              // +0x2C
    int32_t _MapConfigId;       // +0x30
    int32_t _MapConfigIdHome;   // +0x34
    int32_t _TotalSeconds;      // +0x38
    int32_t _FaithPoint;        // +0x3C
    int32_t _SurvivalPoint;     // +0x40
    int32_t _SurvivalTotalPoints; // +0x44
    int32_t _RefreshSurvivalCost; // +0x48
    void* _FuncMap;             // +0x50
};

struct State_Web_CoreUI0_o : BaseReduxState_o
{
    int32_t _CameraMode;                    // +0x20
    bool _IsPause;                          // +0x24
    bool _IsPlanMode;                       // +0x25
    bool _WasPausedBeforePlanMode;          // +0x26
    bool _ChecklistEntryBubbleClicked;      // +0x27
    bool _ChecklistEntryUnlocked;           // +0x28
    void* _TimerPause;                      // +0x30  ReactiveProperty<bool>
    void* _DisplaySeconds;                  // +0x38  ReactiveProperty<float>
    void* _DecreasePerSecond;               // +0x40  ReactiveProperty<int>
    void* _Progress;                        // +0x48
    void* _ProgressSpeed;                   // +0x50
    void* _TimerColor;                      // +0x58
};

struct ReduxStoreLayer_o : Il2CppObject
{
    void* dataTree;                        // +0x10
    void* ReducerDataCaller;               // +0x18
    void* ReducerAsyncDataCaller;          // +0x20
    void* cacheTree;                       // +0x28
    void* ReducerCacheCaller;              // +0x30
    void* ReducerAsyncCacheCaller;         // +0x38
    void* stateTree;                       // +0x40  Dictionary<Type, BaseReduxState>
};

struct ReduxUISystem_o : BaseSingleton_Fields_o
{
    void* _PackageVersion;      // +0x18
    void* _MainCamera;          // +0x20
    void* _UICamera;            // +0x28
    void* reduxActionLayer;     // +0x30
    ReduxStoreLayer_o* reduxStoreLayer; // +0x38
    void* uiViewLayer;          // +0x40
    void* reduxSubscribeLayer;  // +0x48
    void* _logicAdapter;        // +0x50  LogicAdapter（_currentWorldState +0x38）
};

// R3.ReactiveProperty<float> / <int>: currentValue 在 +0x20
struct ReactiveProperty_float_o : Il2CppObject
{
    uint8_t completeState;      // +0x10
    void* error;                // +0x18
    float currentValue;         // +0x20
    void* equalityComparer;     // +0x28
    void* root;                 // +0x30
};

struct ReactiveProperty_int_o : Il2CppObject
{
    uint8_t completeState;      // +0x10
    void* error;                // +0x18
    int32_t currentValue;       // +0x20
    void* equalityComparer;     // +0x28
    void* root;                 // +0x30
};

// Dictionary<TKey, TValue>（IL2CPP 标准布局）
struct Dictionary_o : Il2CppObject
{
    void* _buckets;      // +0x10  Int32[]
    void* _entries;      // +0x18  Entry<TKey,TValue>[]（数组数据从 +0x20 起）
    int32_t _count;      // +0x20
    int32_t _freeList;   // +0x24
    int32_t _freeCount;  // +0x28
    int32_t _version;    // +0x2C
};

// ---------- 接口 ----------

// 初始化 SDK（幂等；HotUpdate.dll 未加载时返回 false，可稍后重试）
bool SLSDK_Init();

// SDK 是否就绪（可安全读取数据）
bool SLSDK_Ready();

// 诊断信息（多行文本，供菜单显示排查：实例链各环节地址）
void SLSDK_DebugInfo(char* buf, size_t len);

// ---------- 玩家属性 ----------
// AttrName: 1=Satiety 饱腹, 2=Morale 心态, 3=Stamina 精力, 4=Health, 5=Vitality 生命
// 返回值是存储值（x1000），如 20000 = 20.000
int32_t SLSDK_GetAttr(int32_t attrName);
// 写入存储值（x1000），写完调 SyncAttr2UI 刷新界面
bool SLSDK_SetAttr(int32_t attrName, int32_t value);
// 读取属性的 Max 上限（存储值 x1000）
int32_t SLSDK_GetAttrMax(int32_t attrName);

// ---------- 金币 ----------
int32_t SLSDK_GetGold();
// 精确设置金币（调游戏 API LeadingRole.SyncGold，等价 mod SetMoney，走游戏逻辑同步显示层）
bool SLSDK_SetGold(int32_t value);
// 增加金币（调游戏 API LeadingRole.AddGold，等价 mod AddGold，自动截断到 int32 上限）
bool SLSDK_AddGold(int32_t amount);

// ---------- 灾变倒计时 ----------
// 显示层当前值（秒）
// 推送显示值给 WebView（走 ReactiveProperty<float>.set_Value）
// 真源 RemainTime（x5 = 显示秒）
// 倒计时流速（冻结=0，正常=5）

// ---------- 灾变阶段（WorldStateType） ----------
// 1=PreDisaster 灾变前, 2=PostDisaster 灾变后
// 写三处：GameTimeManager 逻辑源 + State_Data_Player 数据层 + LogicAdapter UI 缓存

// ---------- 商店库存 ----------
// 恢复商店库存（灾变后 MeltdownHighDemandStock 会把高需求商品库存熔毁为 0 导致售罄）
// 同时修逻辑层 ShopComponent + Redux State_Data_Shop + UI State_Web_ShopUI，并解除售罄显示

// ---------- 物品 / 背包（mod GameApi.AddItem 系列同款） ----------
// 物品视图（panel 用；与 mod BackpackItemView 对应）
typedef struct SLItemView
{
    int64_t InstanceId; // ItemData.InstanceId
    int32_t ConfigId;   // ItemData.ItemConfigId
    int32_t Count;      // ItemData.ItemCount
    float TimeScale;    // ItemData.TimeScale
} SLItemView;

// 添加物品（mod BackpackManager.AddItem 同款：自动找空位/扩容、按 StackLimit 分堆、保质期取配置）
// 返回 true 表示至少加入 1 个；*addedOut 返回实际加入数量（可空）
bool SLSDK_AddItem(int32_t configId, int32_t count, int32_t* addedOut);
// 背包物品列表（按 ownerId 过滤主角背包；返回条目数，items 为空则只返回数量）
int32_t SLSDK_ListBackpackItems(SLItemView* outItems, int32_t maxItems);
// 设置单个物品数量（mod SetBackpackItemCount 同款：超 StackLimit 溢出部分新开堆）
bool SLSDK_SetBackpackItemCount(int64_t instanceId, int32_t newCount);
// 复制物品（mod DuplicateBackpackItem 同款）
bool SLSDK_DuplicateBackpackItem(int64_t instanceId);
// 删除物品（mod RemoveBackpackItem 同款）
bool SLSDK_RemoveBackpackItem(int64_t instanceId);

// ---------- 物品目录（mod ItemCatalog 同款） ----------
typedef struct SLItemInfo
{
    int32_t Id;          // Config_Item.ID
    char Name[96];       // UTF-8（ItemName_Local 优先，无则 ItemName，再兜底 #id）
    int32_t Category;    // 顶级分类
    int32_t SubCategory; // 子分类
    int32_t Price;       // config.price
    int32_t Weight;      // config.weight
} SLItemInfo;

// 构建/刷新物品目录缓存（遍历 ConfigManager._Config_Item_Dict；返回条目数，失败 -1）
int32_t SLSDK_RefreshItemCatalog();

const char* SLSDK_GetItemName(int32_t configId);
// 获取目录缓存（返回条目数；outItems 为空只返回数量，maxItems 限制拷贝条数）
int32_t SLSDK_GetItemCatalog(SLItemInfo* outItems, int32_t maxItems);

// ---------- Buff（mod GameApi.Buff 系列） ----------
typedef struct SLBuffView
{
    int64_t InstanceId; // buffArgsMap key（可移除）；0 = 仅配置生效
    int32_t ConfigId;
    char Name[96];      // UTF-8（Name_Local 优先）
    bool IsGood;
    int32_t Layers;     // BuffCount
    int32_t TimeEndTime;
} SLBuffView;
typedef struct SLBuffConfigView
{
    int32_t ConfigId;
    char Name[96];
    bool IsGood;
    float Duration;     // BuffDuring
} SLBuffConfigView;
// 当前生效 Buff 列表（mod GetBuffs：GetEditorBuffMap + GetEffectiveBuffList 合并去重）
int32_t SLSDK_GetBuffs(SLBuffView* outItems, int32_t maxItems);
// Buff 配置目录（mod GetBuffConfigs：遍历 _Config_Buff_Dict）
int32_t SLSDK_GetBuffConfigs(SLBuffConfigView* outItems, int32_t maxItems);
// 添加 Buff（mod AddBuff：AddBuff(PlayerId, configId) + RequestLeadingRoleBuffRefresh）
bool SLSDK_AddBuff(int32_t configId);
// 按配置 ID 移除（mod RemoveBuffByConfig：生存规划来源转 RemoveSurvivalPlan）
bool SLSDK_RemoveBuffByConfig(int32_t configId);
// 按实例 ID 移除（mod RemoveBuff(long)）
bool SLSDK_RemoveBuff(int64_t instanceId);
// 清空全部 Buff（mod ClearAllBuffs，跳过生存规划）
bool SLSDK_ClearAllBuffs();
// 移除全部负面 Buff（mod RemoveAllNegativeBuffs，跳过生存规划）
int32_t SLSDK_RemoveAllNegativeBuffs();

// ---------- 生存规划（mod GameApi.SurvivalPlan 系列） ----------
typedef struct SLSurvivalPlanView
{
    int32_t TalentId;
    char Name[96];
    char Description[256];
    int32_t Level;
    bool Active;
} SLSurvivalPlanView;
// 已激活的生存规划（mod GetSurvivalPlans：SaveCache 遍历）
int32_t SLSDK_GetSurvivalPlans(SLSurvivalPlanView* outItems, int32_t maxItems);
// 生存规划目录（mod GetSurvivalPlanCatalog：_Config_DailyRandom_Dict + RandomGroup 展开）
int32_t SLSDK_GetSurvivalPlanCatalog(SLSurvivalPlanView* outItems, int32_t maxItems);
bool SLSDK_AddSurvivalPlan(int32_t talentId);
bool SLSDK_RemoveSurvivalPlan(int32_t talentId);

// ---------- 熟练度（mod GameApi.Proficiency 系列） ----------
typedef struct SLProficiencyView
{
    int32_t TypeId;      // 1-6
    char Name[96];
    int32_t Level;
    int32_t Exp;
    int32_t PrevLevelExp;
    int32_t NextLevelExp;
    int32_t MaxLevel;
} SLProficiencyView;
int32_t SLSDK_GetProficiencies(SLProficiencyView* outItems, int32_t maxItems);
bool SLSDK_AddProficiencyExp(int32_t typeId, int32_t amount);
bool SLSDK_AddProficiencyLevels(int32_t typeId, int32_t levels, int32_t* appliedOut);

// ---------- 邻居关系（mod GameApi.Relationship） ----------
typedef struct SLRelationshipView
{
    int32_t Affinity;
    int32_t Tier;
    int32_t MaxAffinity;
    bool Locked;
    char TierName[64];
} SLRelationshipView;
bool SLSDK_GetRelationship(SLRelationshipView* out);
bool SLSDK_SetRelationship(int32_t value);

// ---------- 图鉴 / 成就 ----------
bool SLSDK_UnlockAllCodex();
bool SLSDK_UnlockAllAchievements();

// ---------- 暴露度 / 生存点 / 移动 / 热键 ----------
bool SLSDK_GetExposure(float* current, int32_t* maximum, float* timeExposure, float* moveExposure, bool* running);
bool SLSDK_SetExposure(float value);
// 防暴露开关（mod SetNoExploreExposure：WebGm.LockExploreExposure + 清零）
bool SLSDK_SetNoExploreExposure(bool enabled);
// 防暴露每帧保持（mod ApplyNoExploreExposure：LockExploreExposure 开着时清零 CurExposure）
void SLSDK_ApplyNoExploreExposure();
int32_t SLSDK_GetSurvivalPoints();
bool SLSDK_SetSurvivalPoints(int32_t value);
bool SLSDK_SetMovementBlocked(bool blocked);
void SLSDK_SetHotKeyDisabled(bool disabled);

// ---------- 时间（mod GameApi 时间系列） ----------
int32_t SLSDK_GameDay();
int32_t SLSDK_GameHour();
int32_t SLSDK_GameTotalSeconds();
bool SLSDK_IsClockFrozen();
float SLSDK_RemainCountdownHour();
// 延长准备阶段倒计时（mod ExtendCountdown：AddExtraCountDownTime + 字段回退）
bool SLSDK_ExtendCountdown(int32_t hours);
// 冻结/解冻时间（mod SetTimeFrozen：IsClockFrozen + FrozenOverride）
bool SLSDK_SetTimeFrozen(bool on);
// 冻结每帧保持（mod ApplyFrozenOverride：FrozenOverride 开着时强制 IsClockFrozen）
void SLSDK_ApplyFrozenOverride();

// ---------- 批次4：Detours hook 层（mod Harmony 补丁原生版） ----------
// 安装全部 hook（背包尺寸/无限食物/时间冻结×4/扣时拦截；幂等，panel 首次渲染调用）
bool SLSDK_InstallHooks();
// 安装无限食物显示层 hook（6 Reducer 的 GetShelfLifeText/IsItemExpired + GetShelfLifeDaysRaw，∞ 显示；由 InstallHooks 自动调用）
bool SLSDK_InstallFoodDisplayHooks();

// ---------- 设施耐久（mod GameApi.HomeDurability，门=8 窗=9） ----------
// 设置门窗耐久（mod SetHomeDurability 同款：GetHomeFurnituresBySlotType + SetMax/CurrentDurability）
bool SLSDK_SetHomeDurability(int32_t slotTypeId, int32_t value, int32_t* updatedOut);
// 门窗耐久概要（mod GetHomeDurabilitySummary 同款）
bool SLSDK_GetHomeDurabilitySummary(int32_t slotTypeId, int32_t* count, int32_t* minCurrent, int32_t* maxDurability);

// ---------- 无限食物（mod SetInfiniteFoodShelfLife） ----------
// 开关 + 返回受影响物品数（遍历 items.Cache，Life>0 计数；真正无限由 hook 层实现）
bool SLSDK_SetInfiniteFoodShelfLife(bool enabled, int32_t* affectedOut);
// 当前无限食物开关状态
bool SLSDK_InfiniteFoodEnabled();

// ---------- 背包尺寸 / 负重（mod BackpackManager） ----------
typedef struct SLBagInfo
{
    int32_t Columns;   // Config_Bag.Size[0]
    int32_t Rows;      // Config_Bag.Size[1]
    int32_t Weight;    // bag.GetBagWeight()
    int32_t MaxBurden; // bag.GetMaxBurden()
} SLBagInfo;
bool SLSDK_GetBagInfo(SLBagInfo* out);
// 设置背包尺寸（mod SetSizeCore：改 Config_Bag.Size，记忆原始值用于还原）
bool SLSDK_SetBagSize(int32_t columns, int32_t rows);
bool SLSDK_ResetBagSize();
// 设置最大负重（mod SetMaxBurden 同款：AddExtraBurden 补足到目标）
bool SLSDK_SetMaxBurden(int32_t target);
bool SLSDK_ResetMaxBurden();

// ---------- 属性（mod GameApi 完整版） ----------
// 当前值（mod GetAttr 同款：GetTotalValue_Float 四舍五入）
int32_t SLSDK_GetAttr(int32_t attrName);
// 上限值（mod GetMaxAttr 同款：GetTotalValue_Int）
int32_t SLSDK_GetAttrMax(int32_t attrName);
// 设置当前值（mod SetAttr 同款：Max 联动、上限钳制、SyncAttr2UI）
bool SLSDK_SetAttr(int32_t attrName, int32_t value);
// 设置上限值（mod SetMaxAttr 同款：写 Max 条目 + 联动核心属性 Max/Min + SyncAttr2UI）
bool SLSDK_SetAttrMax(int32_t attrName, int32_t value);
// 属性锁（mod ApplyAttrLocks 同款：hp/精力/饱腹/心态 补满；面板每帧调用）
void SLSDK_ApplyAttrLocks(bool hp, bool stamina, bool satiety, bool morale);
// 移速（mod TryGetMoveSpeed / SetMoveSpeedMultiplier / ResetMoveSpeed 同款）
bool SLSDK_GetMoveSpeed(float* current, float* original, float* multiplier);
bool SLSDK_SetMoveSpeedMultiplier(float multiplier);
bool SLSDK_ResetMoveSpeed();

// ---------- 建造商店（State_Web_BuildShop） ----------
// bs+0x30 = <Gold> RP<int> 商店显示金钱；bs+0x38 = <PackageCapacity> RP<int> 待安装包裹数量
