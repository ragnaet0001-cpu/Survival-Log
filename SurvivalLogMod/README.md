# SurvivalLogMod（BepInEx 插件版，方案 1）

生存日志（Unity IL2CPP + HybridCLR 热更）修改器 —— **BepInEx 6 + C# 托管插件**架构。
参考剑鸦Jy 修改器的实现原理，代码全部自研（不照抄）。

## 架构

| 层 | 技术 |
|----|------|
| 注入 | Doorstop（winhttp.dll + doorstop_config.ini，游戏启动自动加载，免手动注入） |
| 宿主 | BepInEx 6 Unity IL2CPP（BepInEx.Unity.IL2CPP.dll + .NET 6 coreclr） |
| 类型访问 | Il2CppInterop：interop 把游戏类型暴露为 C# 类型，**字段/属性/方法直接调用，零偏移** |
| UI | 游戏自带 **Vuplex WebView**：面板脚本注入游戏主 WebUI 页面（叠加层，不替换游戏页面） |
| 通信 | C#→JS：`ExecuteJavaScript` 推送 JSON；JS→C#：`vuplex.postMessage`（MessageEmitted 事件）+ JS 命令队列轮询双通道 |

## 工程结构

```
SurvivalLogMod\
├── SurvivalLogMod.csproj     # net6.0，引用 BepInEx core + 游戏 interop（E:\...\BepInEx\interop\）
├── Plugin.cs                 # BasePlugin 入口：注册 CheatGUI + 配置热键
├── CheatGUI.cs               # MonoBehaviour 主循环：热键 / 锁定循环 / 冻结兜底
├── GameApi.cs                # 游戏实例链 + 全部功能（类型化访问）
├── WebUiBridge.cs            # Vuplex WebView 桥接（找 prefab / 注入面板 / 双向消息）
└── web\panel.js              # 前端面板（HTML+CSS+JS 自包含，内嵌为资源）
```

## 构建 & 部署

```powershell
dotnet build .\SurvivalLogMod.csproj
# 产物: bin\SurvivalLogMod.dll → 拷贝到:
# E:\Program Files (x86)\Survival Log\BepInEx\plugins\SurvivalLogMod\SurvivalLogMod.dll
```

依赖的 interop 由 BepInEx 首次运行游戏时自动生成（`BepInEx\interop\`）。
**游戏更新后**：BepInEx 检测到 hash 变化自动重新生成 interop，插件代码基本不用动。

## 功能（对应原 C++ 注入版）

- **玩家**：金币（真源 `UserComponent.Money`，走 `SetMoney()` 正规方法同步全层）+ 锁定；
  五维属性（饱腹/心态/精力/健康/生命，内部 x1000，`SetBaseValue_Float` + `SyncAttr2UI`）+ 锁定
- **商店**：商店金钱 / 待安装包裹（`State_Web_BuildShop.Gold/PackageCapacity` 的 ReactiveProperty）+ 锁定；
  恢复商店库存（`ShopComponent.ShopCache` + `BuildShopComponent.StockDict` + `State_Web_ShopUI.ShopCache` 三路，解除灾变熔毁售罄）
- **倒计时**：剩余秒显示 / 设置 / 锁定（`State_Web_CoreUI0.DisplaySeconds`，ReactiveProperty<float>）；
  冻结时间（`GameTimeManager.IsClockFrozen`，1s 兜底强制）；
  灾变阶段切换（`GameTimeManager.WorldStateType` + `State_Data_Player.WorldState`）

## 操作

- 呼出/关闭面板：Insert / F6 / F7 / F8（BepInEx 配置 `cn.miaopasi.survivallog.cfg` 可改）
- 锁定 = 每 0.25s 强制写回固定值（勾选瞬间捕获）

## 与剑鸦Jy 插件共存

两个插件会同时向游戏 WebUI 注入面板（各自独立 DOM），且锁定循环可能互相覆盖。
**测试时建议移走 `plugins\SurvivalLogCheat\` 目录**（改名 .bak 即可）再重启游戏。

## 已知注意

- 若 BepInEx 报 `AbandonedMutexException`（preloader 日志）：完全退出所有游戏进程后重启。
- 游戏主 WebUI 未加载时（主菜单/过场），面板找不到 WebView，进游戏后自动重试（1s 间隔）。
