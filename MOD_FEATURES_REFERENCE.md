# MOD 功能参考（移植用）

> 来源：《生存日志》融合模组整合包 `F:\QQ缓存\QQ文件\heluna小姐姐整合版`（BepInEx 6，游戏 1.0.14956）
> 反编译源码：`D:\Program Files (x86)\LobsterAI\.cowork-temp\dexter_sl_decompile`
> 本文档用于把下列三个 MOD 的功能移植进本 C++ 项目（`D:\Vsyuanma\SurvivalLog`）。

---

## 一、DexterSL_Extra.dll — 生存日志多功能修改器 v3.16.27（菜单键 P）

### 功能列表
- **金币**：`+1000 金币` 按钮；金钱上限钳制（防溢出）。
- **信念点**：`+1000 信念点` 按钮（F8 快捷）；信念点数值搜索定位（只读扫内存对象图并写日志）。
- **跨柜制作联动**：把所有带储物空间的家具（大/中型柜、冰箱等）灌入工具柜白名单（每 15 秒复查）；`IsCraftSource` Harmony 后缀补丁；关键词过滤（置物架/冰箱）。
- **跨柜凑料后自动制造**：工作台按 F9 凑齐材料后自动点制造；厨房按 F9 用当前烹饪栏材料直接开煮。
- **双击配方直接制作**：制造笔记内 0.6 秒内连点两下同一配方 = 自动跨柜凑料并开做。
- **冷藏保鲜**：冰箱/冰柜食物保质期锁定指定天数（默认 999 天），移出自动恢复原保质期。
- **背包保鲜**：非冷藏环境（背包/柜子）食物保质期锁定（默认 99 天），免重登刷新。
- **发电量倍率**：发电机产出按倍数放大（0.1~20）。
- **储电倍率**：电池/储电总容量按倍数放大（0.1~20）。
- **储物格扩容**：置物架/大型柜/中型柜/冰箱等家具格子按倍数放大（×3）。
- **读档属性保护**：自动撤销 dexter.sl 的危险读档属性重写补丁。
- **交易价值提示**：给交易估值结果注入提示（5 个补丁）。
- **WebUI 网格滚动条自动修复**：热更覆盖后自动重打补丁。
- **防穿透**：鼠标不穿透菜单/防吞点击。
- **F10 排错快照**：元数据 + 运行时转储到 `DexterSL_Extra_dump.txt`。
- **面板整合**：可停靠进 dexter.sl 面板；浮动气泡提示开关。

### 移植参考（关键类/方法，反编译自源码）
- `Plugin.cs` 配置项：`MoneyMax` / `FaithMax` / `FaithSearchValue` / `PanelKey` / `ShowToast` / `GridAutoPatch` / `LinkAllStorage` / `LinkAllStoragePatch` / `StorageKeywords` / `AutoMakeAfterGather` / `DoubleClickCraft` / `FridgeFreshEnabled/Days` / `BagFreshEnabled/Days` / `PowerOutputEnabled/Multiplier` / `PowerStorageEnabled/Multiplier` / `ShelfGridEnabled/Multiplier`。
- 跨柜联动：`ToolCabinetConfig.IsCraftSource`（静态）、`Reducer_Web_ToolTable.RA_RecipeClick`、`CraftLinkPatch`、`CraftFlowTracer`。
- 厨房/制造：`Reducer_Web_Cooking.RA_Open/RA_Close`、`CookingContext`、`CrossCraft.cs`。
- 交易价值：`TradeBalanceCalculator.Evaluate`、`Reducer_Web_TradeUI.EvaluateState/SetVerdict`、`WebUI_TradeUI.GetMsg`、`TradeValuePatch`。
- 保鲜：`FridgeFreshManager`、`BagFreshManager`（`SetItemTimeScale`/`UpdateItemTimeScaleByOwner`/`OnHourUpdate`/`CheckAndProcessRot`/`OnMoveBag`/`OnMoveItem` 等）。
- 电力：`PowerManagerPatch.InjectPower`/`CalculateGeneration`/`CalculateCapacity`。
- 防穿透：`MouseBlockPatch.Down/Hold/Up/CursorSet`。

---

## 二、SurvivalLogCheat.dll — 综合面板核心 v1.8.1（菜单键 F2，注入网页 UI）

### 功能列表
- **资金/金币**：setMoney / addGold。
- **生存点/信念点**：setSurvivalPoint / addSurvivalPoint / getPoints。
- **游戏时间**：setTimeFrozen（冻结）/ extendTime（延长）。
- **角色属性**：getAttr / setAttr / setLocks（查看/修改/锁定）。
- **物品目录**：getItems / addItem / dupBag / setBagCount / removeBag（浏览/搜索/添加/复制/改数量/删除）。
- **背包信息**：getBag / getBagInfo / setBagGrid（看包/改格子行列）。
- **技能**：addSkillExp / levelUpSkill。
- **其他**：setExposure（暴露值）/ setMaxBurden（最大负重）/ getDefense（防御）/ getRelationship / setRelationship（关系）。
- **锁定**：lockPower（电力）/ lockDurability（耐久）/ setFurnitureMaxDurability / unlockAllFurniture（全部家具解锁）。
- **增益**：clearBuffs / removeBuffs。
- **Harmony 补丁**：时间冻结（`GameTimeManager.CostTime` 拦截）、强制死亡禁用（`AttributeComponent.CompulsoryDeath` / `CompulsoryDeathByLvUp`）、地图跳转与冻结联动（`GameTimeManager.JumpMap` / `Ac_CoreUI0_MapJump`）、HUD 属性同步缓存（`Ac_CoreUI1_SyncAttr` / `Ac_CoreUI1_SyncAllAttr`）。

### 移植参考（关键类/方法）
- 面板桥：`WebUiBridge.cs`（指令 `setMoney/getItems/addItem/addGold/setAttr/getBag/dupBag/setBagCount/getTimeInfo/removeBuffs/setExposure/addSkillExp/removeBag/lockPower/getPoints/extendTime/clearBuffs/getBagInfo/setBagGrid/getDefense/getRelationship/setRelationship/setMaxBurden/levelUpSkill/setPanelOpen/getClipboard/addSurvivalPoint/setSurvivalPoint/setTimeFrozen/lockDurability/setFurnitureMaxDurability/unlockAllFurniture`）。
- 核心 API：`GameApi.cs`（`SetMoney/AddGold/SetAttr/DuplicateBackpackItem/SetBackpackItemCount/SetExposure/AddSkillExp/SetRelationship/SetMaxBurden/MaintainTimeFreeze/ApplyAttrLocks` 等）。
- 时间冻结：`TimeFreezePatches`、`CostTimePatch`。
- 死亡禁用：`CompulsoryDeathPatch`、`CompulsoryDeathByLvUpPatch`。
- 属性缓存：`AttrSyncCachePatch`、`AttrSyncAllCachePatch`。

---

## 三、SurvivalLogCheat.Extension.dll — 规则扩展 v1.0.1

### 功能列表
- **自动收获**：AutoHarvest。
- **物品堆叠上限放大**：StackLimit（默认关，会干扰工作台/厨房读料）。
- **自由建造**：FreeBuild（区域/镜头/房间三组补丁）。
- **物品无限使用次数**：InfiniteUse / InfiniteDailyUse。
- **种植速度/瞬间种成**：PlantSpeed / InstantPlant。
- **背包原生排序**：NativeBackpackSort。
- **F2 面板指令桥**：WebCommandPatch。

### 移植参考（关键类）
- `AutoHarvestPatch.cs`、`StackLimitPatch.cs`、`CraftStackLimitSyncPatch.cs`、`FreeBuildAreaPatch.cs`、`FreeBuildCameraAreaPatch.cs`、`FreeBuildRoomPatch.cs`、`InfiniteUsePatch.cs`、`InfiniteDailyUsePatch.cs`、`PlantSpeedPatch.cs`、`InstantPlantPatch.cs`、`NativeBackpackSortPatch.cs`、`WebCommandPatch.cs`。

---

## 相关主插件（可选补充）dexter.sl.dll — 懒虫增强版 v1.5.3（菜单键 O）
- 主菜单可调：移动速度 / 最大负重 / 背包容量 / 最大饱腹/心态/精力 / 动作速度 / 烹饪时间 / 暴露值增长 / 精力消耗 倍率。
- WebUI 自动补丁：背包/交易/商店禁用整体缩放 + 网格内部滚动条；工作台/厨房/酿酒/燃料发电机/鼠笼/粉碎机内部滚动。
- 容器白名单大容量：玩家背包、冰箱、收纳架/置物架自动识别放大。
- 精力回补 + 暴露值 detour；五维上限补到新上限。
- 移植参考：`Plugin.cs`（配置项）、`DexterUIBehaviour.cs`（菜单）、`WebUiAutoPatch.cs`、`BagWhitelist.cs`、`ExposureDetour.cs`、`StaminaCostFix.cs`。

---

> 说明：这些 MOD 是用 Harmony + IL2CPP interop 实现（补丁目标均为游戏 HotUpdate 程序集内的类/方法）。植入本 C++ 项目时，对应目标可经本项目 `SurvivalLog\SDK\SurvivalLogSDK.*` 与 `SDK\offsets.h` 里的 Il2Cpp 偏移/函数指针 + detours 挂钩实现，而非 Harmony 特性。
