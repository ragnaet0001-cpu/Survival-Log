# SurvivalLog C++ 迁移计划（mod → 原生 C++）

目标：把 SurvivalLogMod（C# BepInEx）的全部功能迁移到 SurvivalLog 原生 C++ DLL，
删除 C++ 旧实现中 mod 没有的功能（商店库存恢复/建造商店/灾变阶段/倒计时显示推送）。

## 功能全集（来自 功能与调用函数清单.txt + GameApi.cs 通读）

| 分组 | 功能 | mod 实现 | C++ 实现方式 |
|---|---|---|---|
| 金币 | AddGold / SetMoney / CurrentGold | LeadingRole.AddGold / SyncGold / UserComponent.Money | ✅ 已完成（上轮） |
| 物品 | AddItem | BackpackManager.AddItem(int,int,out int,out bool,out string) | runtime_invoke |
| 物品 | 背包列表 | ItemManager.GetItemDataList(long) 遍历 | runtime_invoke + List 遍历 |
| 物品 | 改数量 | ItemManager.GetItemData / SetItemCount + BackpackManager.AddItem 溢出分堆 | runtime_invoke |
| 物品 | 复制 | 复制数量再 AddItem | 组合 |
| 物品 | 删除 | ItemManager.ForceRemoveItem(long) | runtime_invoke |
| 物品 | 背包尺寸 | Harmony ItemManager.GetOwnerBagSize Postfix | Detours hook |
| 物品 | 负重 | BagComponent.GetMaxBurden 等 | 字段/方法 |
| 属性 | GetAttr/GetMaxAttr | AttributeComponent.GetTotalValue_Float/Int | runtime_invoke |
| 属性 | SetAttr（含上限） | SetCurrentAttrDirect 直写字段 + SyncAttr2UI | 字段写 + invoke |
| 属性 | 属性锁（HP/精力/饱腹/心态） | TopUp: 直写 + 锁值 | 字段写 + invoke |
| 属性 | 移速倍率 | SetBaseValue_Float(401, base*mult, true) + SyncAttr2UI | runtime_invoke |
| 设施 | 门窗耐久 | AgentManager.GetHomeFurnituresBySlotType + FurnitureDurabilityComponent.SetMax/CurrentDurability | invoke + List 遍历 |
| 食物 | 无限保质期开关 | 开关状态 + 计数 | 字段/遍历 |
| 食物 | 保质期 hook ×5 | Harmony CheckAndProcessRot / GetShelfLifeText / IsItemExpired / GetShelfLifeDaysRaw | Detours hook |
| 时间 | 游戏天/时/秒 | GameTimeManager.GetDay/GetHour/GetTotalSeconds | runtime_invoke |
| 时间 | 延长倒计时 | CountDownTimer 字段 + AddExtraCountDownTime + NotifyCountDownTimerScaleSync | invoke + 字段 |
| 时间 | 冻结时间 | GameTimeManager.IsClockFrozen + FrozenOverride | 字段写 + hook |
| 时间 | 冻结 hook ×5 | Harmony CostTime×3 / Update / SendAction | Detours hook |
| Buff | 列表 | GetEditorBuffMap / GetEffectiveBuffList 遍历 | 字典/List 遍历 |
| Buff | 添加/移除/清空 | AddBuff(PlayerId,id) / RemoveBuff / RequestLeadingRoleBuffRefresh | runtime_invoke |
| Buff | 配置目录 | ConfigManager._Config_Buff_Dict 遍历 | 字典遍历 |
| 生存规划 | 列表/目录 | SaveCache + FindTalentConfig + GetConfiguredSurvivalPlanIds | 遍历 |
| 生存规划 | 添加/移除 | AddBuff/RemoveBuff + NotifySurvivalPlanningUpdate | runtime_invoke |
| 关系 | 读取/设置 | NeighborRescueManager.GmGetAffinity/AddAffinity | runtime_invoke |
| 图鉴 | 全解锁 | CodexManager.GmUnlockAll | runtime_invoke |
| 成就 | 全解锁 | AchievementManager.GmUnlockAll | runtime_invoke |
| 暴露 | 设置/防暴露 | ExploreManager._CurExposure 字段 + NotifyUI + WebGm.LockExploreExposure | 字段 + invoke |
| 生存点 | 读取/设置 | SurvivalResultsManager.GetSurvivalPoint/AddSurvivalPoint | runtime_invoke |
| 熟练度 | 列表/经验/等级 | ProficiencyManager.GetSnapshot / AddExp / AddLevel | runtime_invoke + 遍历 |
| 杂项 | 移动锁定 | BattleShowWorld._CameraManager.isKeyboardMoveBlocked | 字段 |
| 杂项 | 热键禁用 | BattleShowWorld._HotKeyManager.OnDisableResponse | invoke |
| 帧率 | WebView 帧率 | Harmony WebUILayer.GetTargetWebFrameRateForTier | Detours hook |
| 帧率 | 渲染冻结帧率 | Harmony SettingManager.GetEffectiveTargetFrameRate | Detours hook |

## 删除清单（C++ 旧功能，mod 没有）
- SLSDK_RestoreShopStock（商店库存恢复）
- SLSDK_Get/SetBuildShopGold / PackageCapacity（建造商店）
- SLSDK_Get/SetWorldState（灾变阶段）
- SLSDK_PushCountdownDisplay / SetCountdownRemain / SetDecreasePerSecond（倒计时显示推送）
- 保留：SLSDK_GetGold/SetGold/AddGold、属性（改 mod 实现）、时间（改 mod 实现）

## 实施批次
1. ✅ 批次 1（完成）：SDK 基础设施（ConfigManager/ItemManager/ItemData 解析）+ 物品/背包 + 属性完整版（含 Max/锁/移速）+ 物品目录（ItemCatalog）
2. ✅ 批次 2（完成）：Buff + 生存规划 + 熟练度 + 关系/图鉴/成就 + 暴露/生存点/移动/热键 + 时间（读取/延长/冻结）+ UI 重排（时间/Buff/杂项 tab，删除商店/倒计时旧 tab + 13 个旧 SDK 函数）
3. ✅ 批次 3（完成）：设施耐久（门窗 8/9 + 概要）、无限食物开关（遍历 items.Cache 计数）、背包尺寸（改 Config_Bag.Size + 原始值记忆）、负重（AddExtraBurden 二分补足）
4. ✅ 批次 4（核心 7 hook 完成并验证）：Detours hook 层——GetOwnerBagSize（背包尺寸✓）、CheckAndProcessRot（无限食物核心）、GameTimeManager/CountDownTimer/CountUpTimer.CostTime（冻结拦截×3✓）、GameTimeManager.Update（冻结保持✓）、Ac_Player_UpdateCostHour.SendAction（扣时拦截）；⚠️ 冻结必须走 set_IsClockFrozen setter（mod time.IsClockFrozen=on 有副作用，直写字段无效）；写法用 HookManager.h（HookFunction+CALL_ORIGIN，入口 E9 验证法）；待续：帧率×2（WebUILayer/SettingManager）+ 无限食物显示层（GetShelfLifeText/IsItemExpired/GetShelfLifeDaysRaw ×6 Reducer，需 il2cpp_string_new）
5. ⏳ 批次 5：剩余 UI 完善（物品目录分类分组、熟练度真实名称）

## 关键签名来源
- D:\Vsyuanma\SurvivalLog\SDK_dump\SurvivalLog_PC\DummyDll\HotUpdate.dll（托管元数据）
- D:\Vsyuanma\SurvivalLog\SDK_dump\monodump_HotUpdate.txt
- 签名提取脚本：D:\Program Files (x86)\LobsterAI\.cowork-temp\survivallog_gold\extract_sigs.py
