# SurvivalLog - Survival Log 修改器

Survival Log（生存日志）游戏的修改器：**C++ 原生 DLL**（D3D11 Hook + ImGui 菜单 + il2cpp 运行时 API 直调游戏内部逻辑），功能全部按 mod（C# 版）同款实现。

---

## 功能（9 个菜单 tab）

| Tab | 功能 |
| --- | --- |
| 准备阶段 | 金币显示/增加、游戏时间（天/时刻/总秒/倒计时）、延长倒计时、冻结时间 |
| 物品 | 物品目录搜索（按名称/ID）、添加物品、数量设置 |
| 背包 | 背包物品列表、删除/复制、设置数量、背包尺寸修改、最大负重、柜子/货架扩容（收纳架/置物架/冰箱白名单行数与负重倍率） |
| 杂项 | 邻居好感、图鉴/成就解锁、暴露度、生存点、锁定移动、禁用游戏热键、无限食物保质期、动作速度倍率、烹饪时间倍率、暴露增长倍率 |
| 属性 | 饱腹/心态/精力/生命/Health 当前值与上限、属性锁（每帧补满）、移速倍率 |
| 熟练度 | 6 类熟练度查看、加经验、加等级 |
| 设施 | 全部 10 种槽位类型（小/中/大/挂壁/中央/桌上/床/门/窗/塔防装置）耐久查看与设置、每行锁定（勾选后每帧保持耐久） |
| Buff | 子 tab：当前效果 / 生存规划；当前效果 = Buff 列表/移除 + Buff 目录搜索/添加 + 清空全部/移除负面；生存规划 = 已激活列表/移除 + 目录（全量天赋 548 条）搜索/选中/手动添加 |
| 关于 | 关闭菜单 |

> 菜单结构对齐 mod 前端（资源/生存/其他分组），无透视/自瞄等 mod 之外的功能。

> 2026-08-23 dexter.sl（3DM mod）功能迁移（批次6，最终保留项）+ 崩溃加固（批次7）：
> - 杂项 tab：动作速度倍率（Get_Config_Action hook 返回时改 During/N，ActionType==2 跳过；不做每帧覆盖，避免卡动作）、烹饪时间倍率（OnCookingStart 参数缩放，下限 1s）、暴露增长倍率（ExploreManager 时间/移动速率 + AddExposure 事件缩放）
> - 曾添加后按需求移除：五维上限倍率（属性 tab）、背包负重/行数倍率（背包 tab）——UI 与 SDK 层均已删除
> - 崩溃加固：全部 22 个 Detours hook 函数体包 __try/__except（悬垂对象访问冲突不再崩进程）、PanelFrameUpdate 每帧 il2cpp_thread_attach（渲染线程 GC 期安全，SLSDK_EnsureThreadAttached）、每帧字典遍历加 entries/count 热重载检测、缓存字典容量上限（>1024 自动清空）
>
> 2026-08-24 柜子扩容（参考懒虫增强版白名单机制）+ 无限食物完善：
> - 柜子/货架扩容（背包 tab）：hook ConfigManager.Get_Config_Bag，返回时按名字（收纳架/置物架/冰箱，中英文关键词）自动识别容器进白名单，行数×N + 负重×N 直写 Config_Bag（原始值记忆 per 对象、可还原，缓存上限防读档累积）；玩家背包不受影响（走原有尺寸/负重功能）；倍率默认 1 不启用，重开面板生效
> - 无限食物：物品详情弹窗（ItemDetailPopup）保质期显示 ∞（hook Reducer_Web_ItemDetailPopup.SetShelfLifeParts，把 valueText 换成 ∞，点开物品不再显示真实倒计时）
> - 无限食物崩溃修复：∞ 字符串用 il2cpp_gchandle_new 固定（C++ 全局裸指针不在 IL2CPP GC root 里，游戏更新后会被回收成悬垂，导致打开背包时 GetShelfLifeText 返回悬垂对象崩溃/物品不显示）


---

## 架构

```
SurvivalLog/
├── DXhook/          D3D11 Hook + ImGui 渲染层（Present 每帧无条件调 PanelFrameUpdate；show_window 为 true 时再调 RenderPanel）
├── Hook/            Detours Hook 层（HookManager：背包尺寸/无限食物/时间冻结/柜子扩容等 20+ 个 hook）
├── Menu/Panel/      菜单面板
│   ├── panel.cpp    框架（窗口/侧边栏/分发 + PanelFrameUpdate 每帧系统逻辑 + PanelUpdateLocks 锁定生效）
│   └── Tabs/        9 个 tab 独立文件（TabXxx.cpp，对齐原神项目 Gui 风格）
└── SDK/             il2cpp 运行时封装
    ├── SurvivalLogSDK.h/.cpp  按名解析类/方法 + 全部功能 API
    ├── offsets.h              全免更偏移集中表（运行时动态解析填充）
    └── il2cpp-api.h           GameAssembly.dll 导出 API 声明
```

**SDK 设计**：全部通过 `il2cpp_class_from_name` / `class_get_method_from_name` / `runtime_invoke` 按名解析（类在 HybridCLR 热更程序集 HotUpdate.dll，命名空间 `GameCore.HotUpdate.*`），**不依赖任何 RVA 硬编码**；字段偏移用 `ResolveFieldOffset()` 沿继承链按字段名动态解析。

**偏移免更**：所有偏移集中在 [offsets.h](SurvivalLog/SDK/offsets.h)（`inline size_t` 运行时变量），初始化时按字段名动态解析并直接填充本体；游戏更新后一般无需手动改任何偏移，只有 HybridCLR 元数据遍历不到字段时才需要更新 offsets.h 的兜底值。

**实例获取链示例**：

```
BattleLogicWorld.Instance (BaseSingleton.get_Instance)
  ├─ _AgentManager(+0x40) → GetLeadingRole() → LeadingRole
  │    └─ agentComponentDictionary(+0x48) → AttributeComponent → AttrDict(+0x48) → Attr.BaseValue(+0x10)
  ├─ _GameTimeManager(+0x80) → _Timer(+0x38) → CountDownTimer._RemainTime(+0x10)
  └─ _ProficiencyManager(+0x128) / _ExploreManager(+0x1C8) / _CodexManager(+0x298) ...
ReduxUISystem.Instance → reduxStoreLayer(+0x38) → stateTree(+0x40) → State_Data_Player._Gold(+0x2C)
```

---

## 构建

环境：Visual Studio 2022（v143 工具集，C++20，含 ATL/MFC）

```powershell
"C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" .\SurvivalLog.slnx -p:Configuration=Release -p:Platform=x64 -m -v:m -nologo
```

产物：`x64\Release\SurvivalLog.dll`

> ⚠️ 源码为 UTF-8 BOM 编码（工程无 `/utf-8` 编译选项），改代码必须保留 BOM；面板中文字符串必须 `(const char *)u8"..."` 前缀。

---

## 使用

1. 运行游戏（IL2CPP 单机版），等待进入主界面（HotUpdate.dll 热更程序集加载）
2. 将 `x64\Release\SurvivalLog.dll` 注入游戏进程
3. 菜单默认显示，按 Home 键显示/隐藏菜单；「关于」页的「关闭菜单」按钮也可隐藏
4. SDK 未就绪时面板会提示"等待游戏加载"，自动每 2 秒重试；就绪后自动安装 hook

> 注入工具可自行选择（如 Cheat Engine / x64dbg 等，本仓库不包含注入器）。

---

## 游戏更新适配

1. 正常情况**什么都不用改**：SDK 按类名/方法名解析 + 偏移动态解析，自动适配
2. 看日志确认：出现 `Field offsets resolved` / `ModBatch2 ready` 即偏移解析成功
3. 若 `Struct offset mismatch` 警告或某功能失效：用 Il2CppDumper 重新 dump（`SDK_dump\Il2CppDumper-v6.7.46\`），更新 [offsets.h](SurvivalLog/SDK/offsets.h) 兜底值或结构体布局

---

## 目录结构

```
├── SurvivalLog/            C++ 原生 DLL 工程（本 README 主对象）
├── SDK_dump/               游戏 dump 资料（Il2CppDumper-v6.7.46 工具 / SurvivalLog_PC dump / monodump_HotUpdate.txt 等）
├── x64/                    构建输出（Release\SurvivalLog.dll + .pdb）
├── SurvivalLog.slnx        解决方案
└── push.bat                git 推送脚本
```

