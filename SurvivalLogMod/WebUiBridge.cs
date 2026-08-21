using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Text.Json;
using System.Threading;
using BepInEx;
using BepInEx.Configuration;
using GameCore.HotUpdate;
using GameCore.HotUpdate.ReduxUI;
using GameCore.Scripts;
using GameCore.UnityCore;
using Il2CppInterop.Runtime;
using Il2CppInterop.Runtime.InteropTypes;
using Il2CppInterop.Runtime.InteropTypes.Arrays;
using Il2CppSystem.Threading.Tasks;
using UnityEngine;
using UnityEngine.SceneManagement;
using Vuplex.WebView;

namespace SurvivalLogCheat;

public static class WebUiBridge
{
	private class PendingRead
	{
		public Task<string> Task;

		public Action<string> Callback;

		public double StartedAt;
	}

	private class MemoryData
	{
		public bool LockHp { get; set; }

		public bool LockStamina { get; set; }

		public bool LockSatiety { get; set; }

		public bool LockMorale { get; set; }

		public int LockHpValue { get; set; } = -1;

		public int LockStaminaValue { get; set; } = -1;

		public int LockSatietyValue { get; set; } = -1;

		public int LockMoraleValue { get; set; } = -1;

		public int MaxHealthValue { get; set; } = -1;

		public int MaxStaminaValue { get; set; } = -1;

		public int MaxSatietyValue { get; set; } = -1;

		public int MaxMoraleValue { get; set; } = -1;

		public bool InfiniteFood { get; set; }

		public bool NoExploreExposure { get; set; }

		public bool TimeFrozen { get; set; }

		public float MoveSpeedMultiplier { get; set; } = 1f;

		public int BagColumns { get; set; }

		public int BagRows { get; set; }

		public int BagMaxBurden { get; set; }

		public string ActiveTab { get; set; } = "prepare";
	}

	private static IWebView _web;

	private static double _nextSearch;

	private static double _nextWebHealthCheck;

	private static double _nextTick;

	private static double _nextInjectCheck;

	private static double _nextStatus;

	private static bool _commandReadPending;

	private static volatile bool _commandWakeRequested;

	private static volatile bool _panelCloseRequested;

	private static volatile bool _dragStartRequested;

	private static volatile bool _dragRefreshRequested;

	private static volatile bool _dragEndRequested;

	private static bool _dragActive;

	private static double _dragActiveUntil;

	private static bool _injectConfirmed;

	private static double _nextSceneCheck;

	private static volatile string _currentOp = "idle";

	private static Thread _heartbeat;

	private static bool _heartbeatStarted;

	private static volatile bool _stopHeartbeat;

	private static MemoryData _memory;

	private static bool _memoryLoaded;

	private static bool _memoryApplied;

	private static bool _memoryBagApplied;

	private static double _nextMemoryBagRetry;

	private static int _memoryBagRetryCount;

	private static double _nextMemoryAttrMaxApply;

	private static double _nextMemoryStateCheck;

	private static long _memoryAttrMaxAppliedSource;

	private static bool _memoryAttrMaxErrorLogged;

	private static bool _autoOpenDone;

	private static string _activeTab = "prepare";

	private static string _cachedItemsPayload;

	private static string _lastScene;

	private static BaseWebViewPrefab _gamePrefab;

	private static IWebView _subscribedWeb;

	private static Il2CppSystem.EventHandler<EventArgs<string>> _messageHandler;

	private static Il2CppSystem.Func<Vector2, Camera, bool> _originalValidator;

	private static bool _raycastValidatorStored;

	private static Il2CppSystem.Func<Vector2, Camera, bool> _alwaysTrueValidator;

	private static WebViewStaticMask _storedStaticMask;

	private static WebUILayer _storedWebLayer;

	private static DragMode _originalDragMode;

	private static bool _dragModeStored;

	private static bool _prefabKeyboardEnabled;

	private static bool _prefabClickingEnabled;

	private static bool _webLayerStateStored;

	private static float _canvasAlpha;

	private static bool _canvasBlocksRaycasts;

	private static bool _canvasInteractable;

	private static double _nextInputEnsure;

	private static double _nextPanelInputEnsure;

	private static bool _inputErrorLogged;

	private static bool _webFocusApplied;

	public static bool PanelOpen;

	private static readonly System.Collections.Generic.List<PendingRead> _pending = new System.Collections.Generic.List<PendingRead>();

	public static bool LockHp;

	public static bool LockStamina;

	public static bool LockSatiety;

	public static bool LockMorale;

	public static int LockHpValue = -1;

	public static int LockStaminaValue = -1;

	public static int LockSatietyValue = -1;

	public static int LockMoraleValue = -1;

	public static bool DragActive => _dragActive;

	public static bool AnyLock
	{
		get
		{
			if (!LockHp && !LockStamina && !LockSatiety)
			{
				return LockMorale;
			}
			return true;
		}
	}

	public static void SetOp(string op)
	{
		_currentOp = op ?? "idle";
	}

	public static void StartHeartbeat()
	{
		if (_heartbeatStarted)
		{
			return;
		}
		_heartbeatStarted = true;
		LoadMemory();
		_heartbeat = new Thread((ThreadStart)delegate
		{
			while (!_stopHeartbeat)
			{
				try
				{
					if (_currentOp != "idle")
					{
						GameApi.LogInfo("[心跳/60s] alive op=" + _currentOp);
					}
				}
				catch
				{
				}
				try
				{
					Thread.Sleep(60000);
				}
				catch
				{
					break;
				}
			}
		});
		_heartbeat.IsBackground = true;
		_heartbeat.Start();
		GameApi.LogInfo("[WebUiBridge] 心跳线程已启动");
	}

	public static void StopHeartbeat()
	{
		_stopHeartbeat = true;
	}

	public static void AutoOpenPanel()
	{
		if (!_autoOpenDone)
		{
			_autoOpenDone = true;
			PanelOpen = true;
			try
			{
				ApplyPanelState();
			}
			catch (Exception ex)
			{
				GameApi.LogErr("AutoOpenPanel: " + ex.Message);
			}
			GameApi.LogInfo("[WebUiBridge] 菜单页注入后将自动弹出面板");
		}
	}

	private static void BoostFrameRateForPanel()
	{
	}

	private static void RestoreFrameRateAfterPanel()
	{
	}

	private static int GetConfiguredFrameRate()
	{
		int num = 0;
		try
		{
			num = Plugin.WebViewFrameRateConfig?.Value ?? 0;
		}
		catch
		{
		}
		if (num != 0)
		{
			if (num >= 30)
			{
				if (num <= 240)
				{
					return num;
				}
				return 240;
			}
			return 30;
		}
		return 0;
	}

	public static bool SetFrameRate(int fps, out bool webViewRestartRequired)
	{
		webViewRestartRequired = true;
		if (fps != 0 && (fps < 30 || fps > 240))
		{
			GameApi.LastError = "帧率请选择跟随游戏，或设置 30 到 240 帧";
			return false;
		}
		try
		{
			if (Plugin.WebViewFrameRateConfig != null)
			{
				Plugin.WebViewFrameRateConfig.Value = fps;
				((ConfigEntryBase)Plugin.WebViewFrameRateConfig).ConfigFile.Save();
			}
			if (fps > 0)
			{
				try
				{
					StandaloneWebView.SetTargetFrameRate((uint)fps);
					webViewRestartRequired = false;
				}
				catch (Exception ex)
				{
					GameApi.LogErr("WebView 热切换帧率失败，将在下次初始化时应用: " + ex.Message);
				}
			}
			GameApi.LastError = "";
			GameApi.LogInfo("[FrameRate] WebView=" + ((fps == 0) ? "follow-game" : fps.ToString()) + "; game frame rate remains unchanged");
			return true;
		}
		catch (Exception ex2)
		{
			GameApi.LastError = "设置帧率失败: " + ex2.Message;
			GameApi.LogErr(GameApi.LastError);
			return false;
		}
	}

	private static string MemoryFilePath()
	{
		try
		{
			return Path.Combine(Paths.ConfigPath, "SurvivalLogCheat.memory.json");
		}
		catch
		{
			return null;
		}
	}

	private static void LoadMemory()
	{
		if (_memoryLoaded)
		{
			return;
		}
		_memoryLoaded = true;
		try
		{
			string text = MemoryFilePath();
			if (!string.IsNullOrEmpty(text) && File.Exists(text))
			{
				_memory = JsonSerializer.Deserialize<MemoryData>(File.ReadAllText(text));
				if (_memory != null)
				{
					BackpackManager.SetRememberedSize(_memory.BagColumns, _memory.BagRows);
					BackpackManager.SetRememberedMaxBurden(_memory.BagMaxBurden);
					GameApi.LogInfo("[记忆] 已读取上次保存的设置: " + text);
				}
			}
		}
		catch (Exception ex)
		{
			GameApi.LogErr("[记忆] 读取失败: " + ex.Message);
		}
	}

	internal static void RememberBagSize(int columns, int rows, bool persist)
	{
		int num = ((columns >= 1 && columns <= 20) ? columns : 0);
		int num2 = ((rows >= 1 && rows <= 20) ? rows : 0);
		MemoryData memoryData = _memory ?? new MemoryData();
		bool flag = memoryData.BagColumns != num || memoryData.BagRows != num2;
		memoryData.BagColumns = num;
		memoryData.BagRows = num2;
		_memory = memoryData;
		BackpackManager.SetRememberedSize(num, num2);
		if (!persist || !flag)
		{
			return;
		}
		try
		{
			string text = MemoryFilePath();
			if (!string.IsNullOrEmpty(text))
			{
				Directory.CreateDirectory(Path.GetDirectoryName(text));
				File.WriteAllText(text, JsonSerializer.Serialize(memoryData, new JsonSerializerOptions
				{
					WriteIndented = true
				}));
			}
		}
		catch (Exception ex)
		{
			GameApi.LogErr("[记忆] 保存背包尺寸失败: " + ex.Message);
		}
	}

	internal static void RememberMaxBurden(int target, bool persist)
	{
		int num = Math.Max(0, target);
		MemoryData memoryData = _memory ?? new MemoryData();
		bool flag = memoryData.BagMaxBurden != num;
		memoryData.BagMaxBurden = num;
		_memory = memoryData;
		BackpackManager.SetRememberedMaxBurden(num);
		if (!persist || !flag)
		{
			return;
		}
		try
		{
			string text = MemoryFilePath();
			if (!string.IsNullOrEmpty(text))
			{
				Directory.CreateDirectory(Path.GetDirectoryName(text));
				File.WriteAllText(text, JsonSerializer.Serialize(memoryData, new JsonSerializerOptions
				{
					WriteIndented = true
				}));
			}
		}
		catch (Exception ex)
		{
			GameApi.LogErr("[记忆] 保存最大负重失败: " + ex.Message);
		}
	}

	private static void RememberMoveSpeedMultiplier(float multiplier, bool persist)
	{
		MemoryData memoryData = _memory ?? new MemoryData();
		memoryData.MoveSpeedMultiplier = multiplier;
		_memory = memoryData;
		if (!persist)
		{
			return;
		}
		try
		{
			string text = MemoryFilePath();
			if (!string.IsNullOrEmpty(text))
			{
				Directory.CreateDirectory(Path.GetDirectoryName(text));
				File.WriteAllText(text, JsonSerializer.Serialize(memoryData, new JsonSerializerOptions
				{
					WriteIndented = true
				}));
			}
		}
		catch (Exception ex)
		{
			GameApi.LogErr("[记忆] 保存移动速度失败: " + ex.Message);
		}
	}

	private static void SaveMemory(JsonElement root)
	{
		MemoryData memoryData = _memory ?? new MemoryData();
		memoryData.LockHp = GetBool(root, "lockHp", memoryData.LockHp);
		memoryData.LockStamina = GetBool(root, "lockStamina", memoryData.LockStamina);
		memoryData.LockSatiety = GetBool(root, "lockSatiety", memoryData.LockSatiety);
		memoryData.LockMorale = GetBool(root, "lockMorale", memoryData.LockMorale);
		if (root.TryGetProperty("lockValues", out var value) && value.ValueKind == JsonValueKind.Object)
		{
			memoryData.LockHpValue = GetOptionalInt(value, "Health", memoryData.LockHpValue);
			memoryData.LockStaminaValue = GetOptionalInt(value, "Stamina", memoryData.LockStaminaValue);
			memoryData.LockSatietyValue = GetOptionalInt(value, "Satiety", memoryData.LockSatietyValue);
			memoryData.LockMoraleValue = GetOptionalInt(value, "Morale", memoryData.LockMoraleValue);
		}
		memoryData.LockHpValue = GetOptionalInt(root, "lockHpValue", memoryData.LockHpValue);
		memoryData.LockStaminaValue = GetOptionalInt(root, "lockStaminaValue", memoryData.LockStaminaValue);
		memoryData.LockSatietyValue = GetOptionalInt(root, "lockSatietyValue", memoryData.LockSatietyValue);
		memoryData.LockMoraleValue = GetOptionalInt(root, "lockMoraleValue", memoryData.LockMoraleValue);
		if (root.TryGetProperty("maxValues", out var value2) && value2.ValueKind == JsonValueKind.Object)
		{
			memoryData.MaxHealthValue = GetOptionalInt(value2, "Health", memoryData.MaxHealthValue);
			memoryData.MaxStaminaValue = GetOptionalInt(value2, "Stamina", memoryData.MaxStaminaValue);
			memoryData.MaxSatietyValue = GetOptionalInt(value2, "Satiety", memoryData.MaxSatietyValue);
			memoryData.MaxMoraleValue = GetOptionalInt(value2, "Morale", memoryData.MaxMoraleValue);
		}
		memoryData.MaxHealthValue = GetOptionalInt(root, "maxHealthValue", memoryData.MaxHealthValue);
		memoryData.MaxStaminaValue = GetOptionalInt(root, "maxStaminaValue", memoryData.MaxStaminaValue);
		memoryData.MaxSatietyValue = GetOptionalInt(root, "maxSatietyValue", memoryData.MaxSatietyValue);
		memoryData.MaxMoraleValue = GetOptionalInt(root, "maxMoraleValue", memoryData.MaxMoraleValue);
		memoryData.InfiniteFood = GetBool(root, "infiniteFood", memoryData.InfiniteFood);
		memoryData.NoExploreExposure = GetBool(root, "noExploreExposure", memoryData.NoExploreExposure);
		memoryData.TimeFrozen = GetBool(root, "timeFrozen", memoryData.TimeFrozen);
		memoryData.MoveSpeedMultiplier = GetOptionalFloat(root, "moveSpeedMultiplier", memoryData.MoveSpeedMultiplier);
		memoryData.BagColumns = GetOptionalIntRange(root, "bagColumns", memoryData.BagColumns, 0, 20);
		memoryData.BagRows = GetOptionalIntRange(root, "bagRows", memoryData.BagRows, 0, 20);
		BackpackManager.SetRememberedSize(memoryData.BagColumns, memoryData.BagRows);
		memoryData.BagMaxBurden = GetOptionalIntRange(root, "bagMaxBurden", memoryData.BagMaxBurden, 0, int.MaxValue);
		BackpackManager.SetRememberedMaxBurden(memoryData.BagMaxBurden);
		if (root.TryGetProperty("activeTab", out var value3) && value3.ValueKind == JsonValueKind.String)
		{
			string text = value3.GetString();
			if (!string.IsNullOrEmpty(text))
			{
				memoryData.ActiveTab = text;
			}
		}
		_memory = memoryData;
		_nextMemoryAttrMaxApply = 0.0;
		_memoryAttrMaxAppliedSource = 0L;
		_memoryAttrMaxErrorLogged = false;
		_memoryBagApplied = false;
		_nextMemoryBagRetry = 0.0;
		try
		{
			string text2 = MemoryFilePath();
			if (string.IsNullOrEmpty(text2))
			{
				PushResult("无法定位配置文件目录", ok: false);
				return;
			}
			string directoryName = Path.GetDirectoryName(text2);
			if (!string.IsNullOrEmpty(directoryName))
			{
				Directory.CreateDirectory(directoryName);
			}
			File.WriteAllText(text2, JsonSerializer.Serialize(memoryData, new JsonSerializerOptions
			{
				WriteIndented = true
			}));
			PushResult("设置已保存，重新进入游戏会自动生效", ok: true);
			GameApi.LogInfo("[记忆] 已保存设置: " + text2);
		}
		catch (Exception ex)
		{
			GameApi.LogErr("[记忆] 保存失败: " + ex.Message);
			PushResult("保存失败: " + ex.Message, ok: false);
		}
	}

	private static void PushMemory()
	{
		try
		{
			MemoryData value = _memory ?? new MemoryData();
			PushData("{\"type\":\"memory\",\"state\":" + JsonSerializer.Serialize(value, new JsonSerializerOptions
			{
				PropertyNamingPolicy = JsonNamingPolicy.CamelCase
			}) + "}");
		}
		catch (Exception ex)
		{
			GameApi.LogErr("PushMemory: " + ex.Message);
		}
	}

	private static float GetOptionalFloat(JsonElement root, string name, float fallback)
	{
		if (root.TryGetProperty(name, out var value) && value.ValueKind == JsonValueKind.Number && value.TryGetSingle(out var value2) && float.IsFinite(value2))
		{
			if (!(value2 < 0.5f))
			{
				if (!(value2 > 5f))
				{
					return value2;
				}
				return 5f;
			}
			return 0.5f;
		}
		return fallback;
	}

	private static int GetOptionalIntRange(JsonElement root, string name, int fallback, int minimum, int maximum)
	{
		if (root.TryGetProperty(name, out var value) && value.ValueKind == JsonValueKind.Number && value.TryGetInt32(out var value2) && value2 >= minimum && value2 <= maximum)
		{
			return value2;
		}
		return fallback;
	}

	public static void TryApplyRememberedState()
	{
		if (_memory == null || ((double)Time.unscaledTime < _nextMemoryStateCheck && _memoryApplied && _memoryBagApplied))
		{
			return;
		}
		_nextMemoryStateCheck = (double)Time.unscaledTime + 0.25;
		if (!GameApi.IsReady)
		{
			return;
		}
		ApplyMemoryAttrMaxima();
		if (!_memoryApplied)
		{
			MemoryData memory = _memory;
			try
			{
				LockHp = memory.LockHp;
				LockStamina = memory.LockStamina;
				LockSatiety = memory.LockSatiety;
				LockMorale = memory.LockMorale;
				LockHpValue = memory.LockHpValue;
				LockStaminaValue = memory.LockStaminaValue;
				LockSatietyValue = memory.LockSatietyValue;
				LockMoraleValue = memory.LockMoraleValue;
				if (memory.InfiniteFood)
				{
					GameApi.SetInfiniteFoodShelfLife(enabled: true, out var _);
				}
				if (memory.NoExploreExposure)
				{
					GameApi.SetNoExploreExposure(enabled: true);
				}
				if (memory.TimeFrozen)
				{
					GameApi.SetTimeFrozen(on: true);
				}
				if (memory.MoveSpeedMultiplier >= 0.5f && memory.MoveSpeedMultiplier <= 5f && Math.Abs(memory.MoveSpeedMultiplier - 1f) > 0.001f)
				{
					GameApi.SetMoveSpeedMultiplier(memory.MoveSpeedMultiplier);
				}
				if (AnyLock)
				{
					GameApi.ApplyAttrLocks(LockHp, LockStamina, LockSatiety, LockMorale);
				}
				_memoryApplied = true;
				GameApi.LogInfo("[记忆] 已自动应用上次的开关设置");
			}
			catch (Exception ex)
			{
				GameApi.LogErr("[记忆] 自动应用失败: " + ex.Message);
			}
		}
		ApplyMemoryBag();
	}

	private static void ApplyMemoryAttrMaxima()
	{
		MemoryData memory = _memory;
		if (memory == null || !GameApi.IsReady || (double)Time.unscaledTime < _nextMemoryAttrMaxApply)
		{
			return;
		}
		long attributeSourceId = GameApi.GetAttributeSourceId();
		if (attributeSourceId == 0L || attributeSourceId == _memoryAttrMaxAppliedSource)
		{
			return;
		}
		_nextMemoryAttrMaxApply = (double)Time.unscaledTime + 1.0;
		bool changed = false;
		if ((1u & (EnsureRememberedMaximum((AttrName)105, memory.MaxHealthValue, ref changed) ? 1u : 0u) & (EnsureRememberedMaximum((AttrName)103, memory.MaxStaminaValue, ref changed) ? 1u : 0u) & (EnsureRememberedMaximum((AttrName)101, memory.MaxSatietyValue, ref changed) ? 1u : 0u) & (EnsureRememberedMaximum((AttrName)102, memory.MaxMoraleValue, ref changed) ? 1u : 0u)) != 0)
		{
			_memoryAttrMaxAppliedSource = attributeSourceId;
			_memoryAttrMaxErrorLogged = false;
			if (changed)
			{
				GameApi.LogInfo("[记忆] 已恢复角色属性上限");
			}
		}
		else if (!_memoryAttrMaxErrorLogged)
		{
			_memoryAttrMaxErrorLogged = true;
			GameApi.LogErr("[记忆] 属性上限恢复失败，将继续重试: " + GameApi.LastError);
		}
	}

	private static bool EnsureRememberedMaximum(AttrName name, int target, ref bool changed)
	{
		//IL_0006: Unknown result type (might be due to invalid IL or missing references)
		//IL_0010: Unknown result type (might be due to invalid IL or missing references)
		//IL_001b: Unknown result type (might be due to invalid IL or missing references)
		if (target <= 0)
		{
			return true;
		}
		if (!GameApi.EnsureMaxAttrCanGrow(name))
		{
			return false;
		}
		if (GameApi.GetMaxAttr(name) >= target)
		{
			return true;
		}
		if (!GameApi.SetAttr(name, target))
		{
			return false;
		}
		changed = true;
		return true;
	}

	private static void ApplyMemoryBag()
	{
		if (_memory == null || !GameApi.IsReady || _memoryBagApplied)
		{
			return;
		}
		MemoryData memory = _memory;
		if (memory.BagColumns <= 0 && memory.BagRows <= 0 && memory.BagMaxBurden <= 0)
		{
			_memoryBagApplied = true;
		}
		else
		{
			if ((double)Time.unscaledTime < _nextMemoryBagRetry)
			{
				return;
			}
			_nextMemoryBagRetry = (double)Time.unscaledTime + 2.0;
			try
			{
				bool flag = true;
				int weight;
				int maxBurden;
				int baseColumns;
				int baseRows;
				int naturalMaxBurden;
				if (memory.BagColumns > 0 || memory.BagRows > 0)
				{
					int num = ((memory.BagColumns <= 0) ? 1 : memory.BagColumns);
					int num2 = ((memory.BagRows <= 0) ? 1 : memory.BagRows);
					if (!BackpackManager.TryGetInfo(out var columns, out var rows, out weight, out maxBurden, out baseColumns, out baseRows, out naturalMaxBurden) || columns < num || rows < num2)
					{
						flag &= BackpackManager.SetSize(num, num2, out var error);
						if (!flag)
						{
							GameApi.LogErr("[记忆] 背包尺寸恢复失败: " + error);
						}
					}
				}
				if (memory.BagMaxBurden > 0 && (!BackpackManager.TryGetInfo(out naturalMaxBurden, out baseRows, out baseColumns, out var maxBurden2, out maxBurden, out weight, out var _) || maxBurden2 < memory.BagMaxBurden))
				{
					flag &= BackpackManager.SetMaxBurden(memory.BagMaxBurden, out var error2);
					if (!flag)
					{
						GameApi.LogErr("[记忆] 最大负重恢复失败: " + error2);
					}
				}
				if (flag)
				{
					_memoryBagApplied = true;
					_memoryBagRetryCount = 0;
					GameApi.LogInfo("[记忆] 已恢复背包尺寸和最大负重");
					return;
				}
			}
			catch (Exception ex)
			{
				GameApi.LogErr("[记忆] 恢复背包数据失败: " + ex.Message);
			}
			if (++_memoryBagRetryCount >= 10)
			{
				_memoryBagRetryCount = 0;
				_nextMemoryBagRetry = (double)Time.unscaledTime + 5.0;
				GameApi.LogErr("[记忆] 背包数据暂未就绪，将继续重试");
			}
		}
	}

	private static bool AlwaysTrueRaycast(Vector2 p, Camera c)
	{
		return true;
	}

	private static void RestoreRaycastValidator()
	{
		try
		{
			if (_raycastValidatorStored && (object)_storedStaticMask != null)
			{
				_storedStaticMask.RaycastValidator = _originalValidator;
			}
		}
		catch
		{
		}
		_originalValidator = null;
		_storedStaticMask = null;
		_raycastValidatorStored = false;
	}

	private static void RestoreWebLayerState()
	{
		try
		{
			if (_storedWebLayer != null)
			{
				try
				{
					_storedWebLayer.RestoreLayer();
				}
				catch
				{
				}
				if (_webLayerStateStored && (object)_storedWebLayer.canvasGroup != null)
				{
					_storedWebLayer.canvasGroup.alpha = _canvasAlpha;
					_storedWebLayer.canvasGroup.blocksRaycasts = _canvasBlocksRaycasts;
					_storedWebLayer.canvasGroup.interactable = _canvasInteractable;
				}
			}
		}
		catch
		{
		}
		_webLayerStateStored = false;
		_storedWebLayer = null;
	}

	private static void ResetDragState()
	{
		_dragStartRequested = false;
		_dragRefreshRequested = false;
		_dragEndRequested = false;
		_dragActive = false;
		_dragActiveUntil = 0.0;
	}

	public static void CheckSceneChanged()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		string text = null;
		try
		{
			Scene activeScene = SceneManager.GetActiveScene();
			text = activeScene.name;
		}
		catch
		{
			return;
		}
		if (string.IsNullOrEmpty(text))
		{
			return;
		}
		if (_lastScene != null && _lastScene != text)
		{
			RestoreWebPrefabDragMode();
			RestoreRaycastValidator();
			RestoreWebLayerState();
			UnsubscribeWebMessages();
			_pending.Clear();
			_commandReadPending = false;
			_commandWakeRequested = false;
			ResetDragState();
			_webFocusApplied = false;
			_injectConfirmed = false;
			_memoryApplied = false;
			_nextMemoryStateCheck = 0.0;
			_memoryBagApplied = false;
			_memoryBagRetryCount = 0;
			_nextMemoryBagRetry = 0.0;
			_nextMemoryAttrMaxApply = 0.0;
			_memoryAttrMaxAppliedSource = 0L;
			_memoryAttrMaxErrorLogged = false;
			_nextSearch = 0.0;
			_nextWebHealthCheck = 0.0;
			_cachedItemsPayload = null;
			ItemCatalog.ResetCache();
			_web = null;
			_gamePrefab = null;
			if (PanelOpen)
			{
				GameApi.SetMovementBlocked(blocked: true);
				GameApi.SetHotKeyDisabled(disabled: true);
			}
			GameApi.LogInfo("[WebUiBridge] 场景切换 " + _lastScene + " -> " + text + (PanelOpen ? "（面板保持打开）" : ""));
		}
		_lastScene = text;
	}

	private static void Push(string js)
	{
		IWebView web = _web;
		if (!IsUsableWebView(web))
		{
			InvalidateWebView(web);
			return;
		}
		SetOp("ExecuteJavaScript(Push)");
		try
		{
			web.ExecuteJavaScript(js);
		}
		catch (Exception ex)
		{
			InvalidateWebView(web);
			GameApi.LogInfo("[WebUiBridge] 推送时 WebView 已失效，等待重新绑定: " + ex.Message);
		}
		finally
		{
			SetOp(null);
		}
	}

	private static string Esc(string s)
	{
		if (string.IsNullOrEmpty(s))
		{
			return "";
		}
		StringBuilder stringBuilder = new StringBuilder(s.Length + 16);
		foreach (char c in s)
		{
			switch (c)
			{
			case '\\':
				stringBuilder.Append("\\\\");
				break;
			case '\'':
				stringBuilder.Append("\\'");
				break;
			case '\n':
				stringBuilder.Append("\\n");
				break;
			case '\u2028':
				stringBuilder.Append("\\u2028");
				break;
			case '\u2029':
				stringBuilder.Append("\\u2029");
				break;
			default:
				stringBuilder.Append(c);
				break;
			case '\r':
				break;
			}
		}
		return stringBuilder.ToString();
	}

	private static void PushData(string json)
	{
		Push("window.__slcPush('" + Esc(json) + "');");
	}

	public static void Update()
	{
		if (_panelCloseRequested)
		{
			_panelCloseRequested = false;
			if (PanelOpen)
			{
				PanelOpen = false;
				ApplyPanelState();
				PushData("{\"type\":\"panel\",\"open\":false}");
			}
		}
		if ((double)Time.unscaledTime >= _nextSceneCheck)
		{
			_nextSceneCheck = Time.unscaledTime + 0.5f;
			CheckSceneChanged();
		}
		if (Time.unscaledDeltaTime > 0.4f)
		{
			return;
		}
		EnsureWebView();
		if (_web == null)
		{
			return;
		}
		double num = Time.unscaledTime;
		if (_dragStartRequested)
		{
			_dragStartRequested = false;
			_dragActive = true;
			_dragActiveUntil = num + 3.0;
		}
		if (_dragRefreshRequested)
		{
			_dragRefreshRequested = false;
			_dragActive = true;
			_dragActiveUntil = num + 3.0;
		}
		if (_dragEndRequested)
		{
			_dragEndRequested = false;
			_dragActive = false;
			_nextTick = num;
		}
		else if (_dragActive && num > _dragActiveUntil)
		{
			_dragActive = false;
			_nextTick = num;
		}
		if (!PanelOpen && _injectConfirmed)
		{
			if (!_dragActive)
			{
				ProcessResults();
			}
			return;
		}
		if (!_dragActive)
		{
			if ((_commandWakeRequested || num >= _nextTick) && !_commandReadPending)
			{
				_commandWakeRequested = false;
				_nextTick = num + 0.25;
				_commandReadPending = true;
				if (!IssueRead("JSON.stringify(window.__cheatQueue?window.__cheatQueue.splice(0,32):[])", delegate(string s)
				{
					_commandReadPending = false;
					HandleCommandBatch(s);
				}))
				{
					_commandReadPending = false;
				}
			}
			if (num >= _nextInjectCheck)
			{
				_nextInjectCheck = num + 3.0;
				IssueRead("window.__slcInjected&&document.getElementById('slc-neverlose-host')?1:0", delegate(string s)
				{
					if (s == "1")
					{
						_injectConfirmed = true;
					}
					else
					{
						_injectConfirmed = false;
						InjectPanel();
					}
				});
			}
			if (num >= _nextStatus)
			{
				_nextStatus = num + 1.0;
				PushActiveTabSnapshot();
			}
		}
		if (!_dragActive)
		{
			ProcessResults();
		}
	}

	private static bool IssueRead(string js, Action<string> cb)
	{
		IWebView web = _web;
		if (!IsUsableWebView(web))
		{
			InvalidateWebView(web);
			return false;
		}
		try
		{
			SetOp("ExecuteJavaScript(Read)");
			Task<string> task = web.ExecuteJavaScript(js);
			_pending.Add(new PendingRead
			{
				Task = task,
				Callback = cb,
				StartedAt = Time.unscaledTime
			});
			return true;
		}
		catch (Exception ex)
		{
			InvalidateWebView(web);
			GameApi.LogInfo("[WebUiBridge] 读取时 WebView 已失效，等待重新绑定: " + ex.Message);
			return false;
		}
		finally
		{
			SetOp(null);
		}
	}

	private static void ProcessResults()
	{
		if (_pending.Count == 0)
		{
			return;
		}
		SetOp("ProcessResults");
		try
		{
			for (int num = _pending.Count - 1; num >= 0; num--)
			{
				PendingRead pendingRead = _pending[num];
				bool flag = (double)Time.unscaledTime - pendingRead.StartedAt > 2.0;
				if (pendingRead.Task != null && (((Task)pendingRead.Task).IsCompleted | flag))
				{
					_pending.RemoveAt(num);
					string obj = null;
					try
					{
						if (!flag && !((Task)pendingRead.Task).IsFaulted)
						{
							obj = pendingRead.Task.Result;
						}
					}
					catch (Exception ex)
					{
						GameApi.LogErr("readResult: " + ex.Message);
					}
					try
					{
						pendingRead.Callback?.Invoke(obj);
					}
					catch (Exception ex2)
					{
						GameApi.LogErr("callback: " + ex2.Message);
					}
				}
			}
		}
		finally
		{
			SetOp(null);
		}
	}

	private static void InjectPanel()
	{
		string text = ReadEmbeddedScript("pinyin_pro.js");
		string text2 = ReadEmbeddedScript("cheat_panel.js");
		if (string.IsNullOrEmpty(text2))
		{
			GameApi.LogErr("未找到 cheat_panel.js");
			return;
		}
		string text3 = ReadEmbeddedBase64("support.jpg");
		text2 = text2.Replace("__SLC_SUPPORT_IMAGE__", string.IsNullOrEmpty(text3) ? "" : ("data:image/jpeg;base64," + text3));
		string text4 = ReadEmbeddedBase64("avatar.png");
		text2 = text2.Replace("__SLC_AVATAR_IMAGE__", string.IsNullOrEmpty(text4) ? "" : ("data:image/png;base64," + text4));
		string text5 = text + "\n" + text2;
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append('\'');
		string text6 = text5;
		foreach (char c in text6)
		{
			switch (c)
			{
			case '\\':
				stringBuilder.Append("\\\\");
				break;
			case '\'':
				stringBuilder.Append("\\'");
				break;
			case '\n':
				stringBuilder.Append("\\n");
				break;
			default:
				stringBuilder.Append(c);
				break;
			case '\r':
				break;
			}
		}
		stringBuilder.Append('\'');
		string s = "{\"type\":\"panel\",\"open\":" + (PanelOpen ? "true" : "false") + "}";
		Push("if(!(window.__slcInjected&&document.getElementById('slc-neverlose-host'))){var el=document.createElement('script');el.type='text/javascript';el.textContent=" + stringBuilder.ToString() + ";document.documentElement.appendChild(el);}window.__slcPush&&window.__slcPush('" + Esc(s) + "');");
		if (PanelOpen)
		{
			ApplyPanelState();
		}
		GameApi.LogInfo("[WebUiBridge] 已注入作弊面板脚本（含拼音库）");
	}

	private static string ReadEmbeddedScript(string filename)
	{
		try
		{
			using Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("SurvivalLogCheat." + filename);
			if (stream == null)
			{
				GameApi.LogErr("Embedded resource not found: " + filename);
				return null;
			}
			using StreamReader streamReader = new StreamReader(stream, Encoding.UTF8);
			return streamReader.ReadToEnd();
		}
		catch (Exception ex)
		{
			GameApi.LogErr("Read embedded resource " + filename + ": " + ex.Message);
			return null;
		}
	}

	private static string ReadEmbeddedBase64(string filename)
	{
		try
		{
			using Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("SurvivalLogCheat." + filename);
			if (stream == null)
			{
				GameApi.LogErr("Embedded resource not found: " + filename);
				return null;
			}
			using MemoryStream memoryStream = new MemoryStream();
			stream.CopyTo(memoryStream);
			return Convert.ToBase64String(memoryStream.ToArray());
		}
		catch (Exception ex)
		{
			GameApi.LogErr("Read embedded resource " + filename + ": " + ex.Message);
			return null;
		}
	}

	public static int GetLockValue(AttrName name)
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_001c: Expected I4, but got Unknown
		return ((int)name - 1) switch
		{
			4 => LockHpValue, 
			2 => LockStaminaValue, 
			0 => LockSatietyValue, 
			1 => LockMoraleValue, 
			_ => -1, 
		};
	}

	private static void UpdateLockTargetAfterManualSet(AttrName name, int value)
	{
		//IL_0006: Unknown result type (might be due to invalid IL or missing references)
		//IL_0008: Unknown result type (might be due to invalid IL or missing references)
		//IL_0022: Expected I4, but got Unknown
		MemoryData memory = _memory;
		switch ((int)name - 1)
		{
		case 4:
			if (LockHp)
			{
				LockHpValue = value;
				if (memory != null)
				{
					memory.LockHpValue = value;
				}
			}
			break;
		case 2:
			if (LockStamina)
			{
				LockStaminaValue = value;
				if (memory != null)
				{
					memory.LockStaminaValue = value;
				}
			}
			break;
		case 0:
			if (LockSatiety)
			{
				LockSatietyValue = value;
				if (memory != null)
				{
					memory.LockSatietyValue = value;
				}
			}
			break;
		case 1:
			if (LockMorale)
			{
				LockMoraleValue = value;
				if (memory != null)
				{
					memory.LockMoraleValue = value;
				}
			}
			break;
		case 3:
			break;
		}
	}

	private static void UpdateMaximumTargetAfterManualSet(AttrName name, int value)
	{
		//IL_000a: Unknown result type (might be due to invalid IL or missing references)
		//IL_000d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0027: Expected I4, but got Unknown
		MemoryData memory = _memory;
		if (memory != null)
		{
			switch ((int)name - 101)
			{
			case 4:
				memory.MaxHealthValue = value;
				break;
			case 2:
				memory.MaxStaminaValue = value;
				break;
			case 0:
				memory.MaxSatietyValue = value;
				break;
			case 1:
				memory.MaxMoraleValue = value;
				break;
			}
			_nextMemoryAttrMaxApply = (double)Time.unscaledTime + 1.0;
		}
	}

	private static void HandleCommandBatch(string json)
	{
		if (string.IsNullOrEmpty(json) || json == "[]")
		{
			return;
		}
		try
		{
			using JsonDocument jsonDocument = JsonDocument.Parse(json);
			JsonElement rootElement = jsonDocument.RootElement;
			if (rootElement.ValueKind != JsonValueKind.Array)
			{
				HandleCommand(json);
				return;
			}
			foreach (JsonElement item in rootElement.EnumerateArray())
			{
				HandleCommand((item.ValueKind == JsonValueKind.String) ? item.GetString() : item.GetRawText());
			}
		}
		catch (Exception ex)
		{
			GameApi.LogErr("HandleCommandBatch: " + ex.Message);
		}
	}

	private static void OnWebMessage(object sender, EventArgs<string> args)
	{
		if (args != null)
		{
			switch (args.Value)
			{
			case "slc-close-panel":
				_panelCloseRequested = true;
				break;
			case "slc-wake":
				_commandWakeRequested = true;
				break;
			case "slc-drag-start":
				_dragStartRequested = true;
				break;
			case "slc-drag-active":
				_dragRefreshRequested = true;
				break;
			case "slc-drag-end":
				_dragEndRequested = true;
				_commandWakeRequested = true;
				break;
			}
		}
	}

	private static void SubscribeWebMessages()
	{
		if (!IsUsableWebView(_web) || _subscribedWeb == _web)
		{
			return;
		}
		UnsubscribeWebMessages();
		try
		{
			if (_messageHandler == null)
			{
				_messageHandler = DelegateSupport.ConvertDelegate<Il2CppSystem.EventHandler<EventArgs<string>>>((Delegate)new Action<object, EventArgs<string>>(OnWebMessage));
			}
			var baseWeb = _web.TryCast<Vuplex.WebView.Internal.BaseWebView>();
			if (baseWeb != null) baseWeb.MessageEmitted += _messageHandler;
			_subscribedWeb = _web;
		}
		catch (Exception ex)
		{
			GameApi.LogErr("SubscribeWebMessages: " + ex.Message);
			_subscribedWeb = null;
		}
	}

	private static void UnsubscribeWebMessages()
	{
		if (_subscribedWeb == null || _messageHandler == null)
		{
			_subscribedWeb = null;
			return;
		}
		try
		{
			var baseWeb = _subscribedWeb.TryCast<Vuplex.WebView.Internal.BaseWebView>();
			if (baseWeb != null) baseWeb.MessageEmitted -= _messageHandler;
		}
		catch
		{
		}
		_subscribedWeb = null;
	}

	private static void EnsureWebView()
	{
		double num = Time.unscaledTime;
		if (_web != null && num >= _nextWebHealthCheck)
		{
			_nextWebHealthCheck = num + 0.25;
			if ((object)_gamePrefab == null || !IsUsableWebView(_web))
			{
				InvalidateWebView(_web);
			}
		}
		if (_web == null && (object)_gamePrefab != null)
		{
			_gamePrefab = null;
		}
		double num2 = num;
		if (num2 < _nextSearch)
		{
			return;
		}
		_nextSearch = num2 + 2.0;
		try
		{
			BaseWebViewPrefab val = FindGamePrefab();
			IWebView val2 = ((val != null) ? val.WebView : null);
			if (!((object)val != null) || !IsUsableWebView(val2))
			{
				return;
			}
			if (!SameWebView(_web, val2))
			{
				RestoreWebPrefabDragMode();
				UnsubscribeWebMessages();
				_pending.Clear();
				_commandReadPending = false;
				_commandWakeRequested = false;
				ResetDragState();
				_web = val2;
				_gamePrefab = val;
				_nextWebHealthCheck = 0.0;
				_webFocusApplied = false;
				_injectConfirmed = false;
				_nextInjectCheck = 0.0;
				SubscribeWebMessages();
				GameApi.LogInfo("[WebUiBridge] 已定位或重新绑定游戏 WebView");
				if (PanelOpen)
				{
					ApplyPanelState();
				}
			}
			else
			{
				_gamePrefab = val;
			}
		}
		catch (Exception ex)
		{
			GameApi.LogErr("[WebUiBridge] 定位 WebView 失败: " + ex.Message);
		}
	}

	private static bool SameWebView(IWebView left, IWebView right)
	{
		if (left == right)
		{
			return true;
		}
		if (left == null || right == null)
		{
			return false;
		}
		try
		{
			return ((Il2CppObjectBase)left).Pointer == ((Il2CppObjectBase)right).Pointer;
		}
		catch
		{
			return false;
		}
	}

	private static bool IsUsableWebView(IWebView web)
	{
		if (web == null)
		{
			return false;
		}
		try
		{
			return !web.IsDisposed && web.IsInitialized;
		}
		catch
		{
			return false;
		}
	}

	private static void InvalidateWebView(IWebView web)
	{
		if (_web != null && (web == null || SameWebView(_web, web)))
		{
			RestoreWebPrefabDragMode();
			UnsubscribeWebMessages();
			_pending.Clear();
			_commandReadPending = false;
			_commandWakeRequested = false;
			ResetDragState();
			_web = null;
			_gamePrefab = null;
			_webFocusApplied = false;
			_injectConfirmed = false;
			_nextSearch = 0.0;
			_nextWebHealthCheck = 0.0;
			_nextInjectCheck = 0.0;
		}
	}

	private static BaseWebViewPrefab FindGamePrefab()
	{
		try
		{
			ReduxUISystem instance = BaseSingleton<ReduxUISystem>.Instance;
			object obj;
			if (instance == null)
			{
				obj = null;
			}
			else
			{
				WebUILayer webUILayer = instance.GetWebUILayer();
				obj = ((webUILayer != null) ? webUILayer.canvasWebViewPrefab : null);
			}
			CanvasWebViewPrefab val = (CanvasWebViewPrefab)obj;
			if ((object)val != null && IsUsableWebView(((BaseWebViewPrefab)val).WebView))
			{
				return (BaseWebViewPrefab)(object)val;
			}
		}
		catch
		{
		}
		try
		{
			Il2CppArrayBase<CanvasWebViewPrefab> val2 = UnityEngine.Object.FindObjectsOfType<CanvasWebViewPrefab>(true);
			if (val2 != null)
			{
				for (int i = 0; i < val2.Length; i++)
				{
					CanvasWebViewPrefab val3 = val2[i];
					if ((object)val3 != null && IsUsableWebView(((BaseWebViewPrefab)val3).WebView))
					{
						return (BaseWebViewPrefab)(object)val3;
					}
				}
			}
			Il2CppArrayBase<WebViewPrefab> val4 = UnityEngine.Object.FindObjectsOfType<WebViewPrefab>(true);
			if (val4 != null)
			{
				for (int j = 0; j < val4.Length; j++)
				{
					WebViewPrefab val5 = val4[j];
					if ((object)val5 != null && IsUsableWebView(((BaseWebViewPrefab)val5).WebView))
					{
						return (BaseWebViewPrefab)(object)val5;
					}
				}
			}
			Il2CppArrayBase<BaseWebViewPrefab> val6 = UnityEngine.Object.FindObjectsOfType<BaseWebViewPrefab>(true);
			if (val6 != null)
			{
				for (int k = 0; k < val6.Length; k++)
				{
					BaseWebViewPrefab val7 = val6[k];
					if ((object)val7 != null && IsUsableWebView(val7.WebView))
					{
						return val7;
					}
				}
			}
			Il2CppArrayBase<BaseWebViewPrefab> val8 = Resources.FindObjectsOfTypeAll<BaseWebViewPrefab>();
			if (val8 != null)
			{
				for (int l = 0; l < val8.Length; l++)
				{
					BaseWebViewPrefab val9 = val8[l];
					if ((object)val9 != null && IsUsableWebView(val9.WebView))
					{
						return val9;
					}
				}
			}
		}
		catch (Exception ex)
		{
			GameApi.LogErr("[WebUiBridge] FindGamePrefab: " + ex.Message);
		}
		return null;
	}

	private static void HandleCommand(string json)
	{
		//IL_0c31: Unknown result type (might be due to invalid IL or missing references)
		//IL_0d29: Unknown result type (might be due to invalid IL or missing references)
		//IL_0c40: Unknown result type (might be due to invalid IL or missing references)
		//IL_0c61: Unknown result type (might be due to invalid IL or missing references)
		//IL_0c8b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0d38: Unknown result type (might be due to invalid IL or missing references)
		//IL_0d59: Unknown result type (might be due to invalid IL or missing references)
		//IL_0d83: Unknown result type (might be due to invalid IL or missing references)
		if (string.IsNullOrEmpty(json))
		{
			return;
		}
		string text = "";
		try
		{
			using JsonDocument jsonDocument = JsonDocument.Parse(json);
			JsonElement rootElement = jsonDocument.RootElement;
			if (!rootElement.TryGetProperty("cmd", out var value) || value.ValueKind != JsonValueKind.String)
			{
				return;
			}
			text = value.GetString() ?? "";
			AttrName attr2;
			switch (text)
			{
			case "ready":
				PushMemory();
				PushActiveTabSnapshot();
				break;
			case "saveMemory":
				SaveMemory(rootElement);
				break;
			case "setActiveTab":
			{
				string text4 = (rootElement.TryGetProperty("tab", out var value28) ? value28.GetString() : null);
				switch (text4)
				{
				case "prepare":
				case "money":
				case "items":
				case "bag":
				case "time":
				case "attributes":
				case "proficiency":
				case "facilities":
				case "about":
				case "resources":
				case "buffs":
					_activeTab = text4;
					PushActiveTabSnapshot();
					break;
				}
				break;
			}
			case "setFrameRate":
			{
				if (TryGetInt(rootElement, "fps", 0, 240, "帧率", out var value21))
				{
					bool flag20 = SetFrameRate(value21, out var webViewRestartRequired);
					PushResult((!flag20) ? GameApi.LastError : ((!webViewRestartRequired) ? ((value21 == 0) ? "已切换为跟随游戏帧数" : $"已切换到 {value21} 帧") : ((value21 == 0) ? "已保存跟随游戏帧数，当前界面已初始化，重启游戏后生效" : $"已保存 {value21} 帧，当前界面已初始化，重启游戏后生效")), flag20);
				}
				break;
			}
			case "getItems":
				PushItems();
				break;
			case "setMoney":
			{
				if (TryGetInt(rootElement, "amount", 0, int.MaxValue, "金币数量", out var value11))
				{
					GameApi.SetMoney(value11);
					PushResult((GameApi.LastError == "") ? "已设置金币" : GameApi.LastError, GameApi.LastError == "", withGold: true);
				}
				break;
			}
			case "addGold":
			{
				if (TryGetInt(rootElement, "amount", 0, int.MaxValue, "金币数量", out var value19))
				{
					bool flag16 = GameApi.AddGold(value19);
					string text2 = (GameApi.LastGoldCapped ? $"已增加 {GameApi.LastGoldAdded} 金币（余额已达上限）" : "已增加金币");
					PushResult(flag16 ? text2 : GameApi.LastError, flag16, withGold: true);
				}
				break;
			}
			case "addItem":
			{
				if (TryGetInt(rootElement, "id", 1, int.MaxValue, "物品编号", out var value8) && TryGetInt(rootElement, "count", 1, 999, "物品数量", out var value9))
				{
					bool flag6 = GameApi.AddItem(value8, value9);
					bool flag7 = flag6 && string.IsNullOrEmpty(GameApi.LastError) && GameApi.LastAddedCount == value9;
					PushResult(flag7 ? $"已加入 {GameApi.LastAddedCount} 个" : (flag6 ? $"{GameApi.LastError}，实际加入 {GameApi.LastAddedCount}/{value9} 个" : GameApi.LastError), flag7);
					if (flag6)
					{
						PushBag();
						PushBagInfo();
					}
				}
				break;
			}
			case "setLocks":
				LockHp = GetBool(rootElement, "hp", def: false);
				LockStamina = GetBool(rootElement, "stamina", def: false);
				LockSatiety = GetBool(rootElement, "satiety", def: false);
				LockMorale = GetBool(rootElement, "morale", def: false);
				LockHpValue = GetOptionalInt(rootElement, "hpValue", LockHpValue);
				LockStaminaValue = GetOptionalInt(rootElement, "staminaValue", LockStaminaValue);
				LockSatietyValue = GetOptionalInt(rootElement, "satietyValue", LockSatietyValue);
				LockMoraleValue = GetOptionalInt(rootElement, "moraleValue", LockMoraleValue);
				break;
			case "getAttr":
				PushAttrs();
				break;
			case "setAttr":
			{
				string text5 = (rootElement.TryGetProperty("a", out var value31) ? value31.GetString() : null);
				if (!TryParseAttr(text5, out var attr3) || !TryGetInt(rootElement, "v", 0, 1000000, "属性值", out var value32))
				{
					if (text5 == null || !TryParseAttr(text5, out attr2))
					{
						PushResult("未知的角色属性", ok: false);
					}
					break;
				}
				bool flag32 = GameApi.SetAttr(attr3, value32);
				if (flag32)
				{
					UpdateLockTargetAfterManualSet(attr3, value32);
					GameApi.LogInfo($"[Attr] Set {attr3} current: requested={value32}, actual={GameApi.GetAttr(attr3)}");
				}
				PushResult(flag32 ? "属性已设置" : GameApi.LastError, flag32);
				PushAttrs();
				break;
			}
			case "setAttrMax":
			{
				string text3 = (rootElement.TryGetProperty("a", out var value24) ? value24.GetString() : null);
				if (!TryParseMaxAttr(text3, out var attr) || !TryGetInt(rootElement, "v", 1, 1000000, "属性上限", out var value25))
				{
					if (text3 == null || !TryParseMaxAttr(text3, out attr2))
					{
						PushResult("未知的角色属性上限", ok: false);
					}
					break;
				}
				bool flag27 = GameApi.SetAttr(attr, value25);
				if (flag27)
				{
					UpdateMaximumTargetAfterManualSet(attr, value25);
					GameApi.LogInfo($"[Attr] Set {attr}: requested={value25}, actual={GameApi.GetMaxAttr(attr)}");
				}
				PushResult(flag27 ? "属性上限已设置" : GameApi.LastError, flag27);
				PushAttrs();
				break;
			}
			case "setMoveSpeed":
			{
				if (TryGetFloat(rootElement, "multiplier", 0.5f, 5f, "移动速度倍率", out var value4))
				{
					bool flag3 = GameApi.SetMoveSpeedMultiplier(value4);
					if (flag3)
					{
						RememberMoveSpeedMultiplier(value4, persist: false);
					}
					PushResult(flag3 ? $"移动速度已设为 {value4:0.##} 倍" : GameApi.LastError, flag3);
					PushMoveSpeed();
				}
				break;
			}
			case "resetMoveSpeed":
			{
				bool flag24 = GameApi.ResetMoveSpeed();
				if (flag24)
				{
					RememberMoveSpeedMultiplier(1f, persist: true);
				}
				PushResult(flag24 ? "移动速度已恢复" : GameApi.LastError, flag24);
				PushMoveSpeed();
				break;
			}
			case "setDoorDurability":
			{
				if (TryGetInt(rootElement, "value", 1, 100000000, "门耐久", out var value20))
				{
					bool flag18 = GameApi.SetHomeDurability(8, value20, out var updated);
					PushResult(flag18 ? $"已修改 {updated} 扇门" : GameApi.LastError, flag18);
					PushFacilities();
				}
				break;
			}
			case "setWindowDurability":
			{
				if (TryGetInt(rootElement, "value", 1, 100000000, "窗耐久", out var value34))
				{
					bool flag34 = GameApi.SetHomeDurability(9, value34, out var updated2);
					PushResult(flag34 ? $"已修改 {updated2} 扇窗" : GameApi.LastError, flag34);
					PushFacilities();
				}
				break;
			}
			case "setInfiniteFood":
			{
				bool flag28 = GetBool(rootElement, "on", def: false);
				bool flag29 = GameApi.SetInfiniteFoodShelfLife(flag28, out var affected);
				PushResult((!flag29) ? GameApi.LastError : (flag28 ? $"无限食物保质期已开启，已接管 {affected} 件物品" : "食物保质期已恢复正常"), flag29);
				PushFacilities();
				break;
			}
			case "unlockAllCodex":
			{
				bool flag19 = GameApi.UnlockAllCodex(out var details2);
				PushResult(flag19 ? details2 : GameApi.LastError, flag19);
				break;
			}
			case "unlockAllAchievements":
			{
				bool flag11 = GameApi.UnlockAllAchievements(out var details);
				PushResult(flag11 ? details : GameApi.LastError, flag11);
				break;
			}
			case "getBag":
				PushBag();
				break;
			case "dupBag":
			{
				if (TryGetLong(rootElement, "instanceId", "背包物品编号", out var value3))
				{
					bool flag2 = GameApi.DuplicateBackpackItem(value3);
					PushResult(flag2 ? $"已复制 {GameApi.LastAddedCount} 个" : GameApi.LastError, flag2);
					PushBag();
				}
				break;
			}
			case "setBagCount":
			{
				if (TryGetLong(rootElement, "instanceId", "背包物品编号", out var value26) && TryGetInt(rootElement, "count", 1, 999999, "物品数量", out var value27))
				{
					GameApi.SetBackpackItemCount(value26, value27);
					PushResult((GameApi.LastError == "") ? "已修改数量" : GameApi.LastError, GameApi.LastError == "");
					PushBag();
				}
				break;
			}
			case "removeBag":
			{
				if (TryGetLong(rootElement, "instanceId", "背包物品编号", out var value23))
				{
					GameApi.RemoveBackpackItem(value23);
					PushResult((GameApi.LastError == "") ? "已删除" : GameApi.LastError, GameApi.LastError == "");
					PushBag();
				}
				break;
			}
			case "getBagInfo":
				PushBagInfo();
				break;
			case "setBagSize":
			{
				if (TryGetInt(rootElement, "columns", 1, 20, "背包列数", out var value16) && TryGetInt(rootElement, "rows", 1, 20, "背包行数", out var value17))
				{
					bool flag14 = BackpackManager.SetSize(value16, value17, out var error3);
					PushResult(flag14 ? "背包尺寸已更新" : error3, flag14);
					PushBagInfo();
				}
				break;
			}
			case "resetBagSize":
			{
				if (!BackpackManager.ResetSize(out var error2))
				{
					PushResult(error2, ok: false);
					PushBagInfo();
				}
				else
				{
					PushResult("已恢复原始背包尺寸", ok: true);
					PushBagInfo();
				}
				break;
			}
			case "setMaxBurden":
			{
				if (TryGetInt(rootElement, "value", 0, int.MaxValue, "最大负重", out var value10))
				{
					bool flag8 = BackpackManager.SetMaxBurden(value10, out var error);
					PushResult(flag8 ? "最大负重已更新" : error, flag8);
					PushBagInfo();
				}
				break;
			}
			case "resetMaxBurden":
				BackpackManager.ResetMaxBurden();
				RememberMaxBurden(0, persist: true);
				PushResult("已恢复原始最大负重", ok: true);
				PushBagInfo();
				break;
			case "getTimeInfo":
				PushTimeInfo();
				break;
			case "extendTime":
			{
				if (TryGetInt(rootElement, "hours", 1, 10000, "延长时间", out var value30))
				{
					bool flag31 = GameApi.ExtendCountdown(value30);
					PushResult(flag31 ? $"已延长 {value30} 小时" : GameApi.LastError, flag31);
					PushTimeInfo();
				}
				break;
			}
			case "setTimeFrozen":
			{
				bool flag22 = GetBool(rootElement, "on", def: false);
				bool flag23 = GameApi.SetTimeFrozen(flag22);
				if (flag23 && _memory != null)
				{
					_memory.TimeFrozen = flag22;
				}
				PushResult((!flag23) ? GameApi.LastError : (flag22 ? "时间已冻结" : "时间已恢复"), flag23);
				PushTimeInfo();
				break;
			}
			case "getBuffs":
				PushBuffs();
				break;
			case "getBuffConfigs":
				PushBuffConfigs();
				break;
			case "getSurvivalPlans":
				PushSurvivalPlans();
				break;
			case "getSurvivalPlanCatalog":
				PushSurvivalPlanCatalog();
				break;
			case "addSurvivalPlan":
			{
				if (TryGetInt(rootElement, "talentId", 1, int.MaxValue, "生存规划 ID", out var value18))
				{
					bool flag15 = GameApi.AddSurvivalPlan(value18);
					PushResult(flag15 ? "生存规划已添加" : GameApi.LastError, flag15);
					PushSurvivalPlans();
					PushSurvivalPlanCatalog();
					PushBuffs();
				}
				break;
			}
			case "removeSurvivalPlan":
			{
				if (TryGetInt(rootElement, "talentId", 1, int.MaxValue, "生存规划 ID", out var value15))
				{
					bool flag12 = GameApi.RemoveSurvivalPlan(value15);
					PushResult(flag12 ? "生存规划已移除" : GameApi.LastError, flag12);
					PushSurvivalPlans();
					PushSurvivalPlanCatalog();
					PushBuffs();
				}
				break;
			}
			case "addBuff":
			{
				if (TryGetInt(rootElement, "configId", 1, int.MaxValue, "Buff 配置 ID", out var value12))
				{
					bool flag9 = GameApi.AddBuff(value12);
					PushResult(flag9 ? "Buff 已添加" : GameApi.LastError, flag9);
					PushBuffs();
					PushSurvivalPlans();
					PushSurvivalPlanCatalog();
				}
				break;
			}
			case "removeBuffByConfig":
			{
				if (TryGetInt(rootElement, "configId", 1, int.MaxValue, "Buff 閰嶇疆 ID", out var value5))
				{
					bool flag4 = GameApi.RemoveBuffByConfig(value5);
					PushResult(flag4 ? "Buff 已移除" : GameApi.LastError, flag4);
					PushBuffs();
					PushSurvivalPlans();
					PushSurvivalPlanCatalog();
				}
				break;
			}
			case "removeBuff":
			{
				if (TryGetLong(rootElement, "instanceId", "Buff 实例 ID", out var value33))
				{
					bool flag33 = GameApi.RemoveBuff(value33);
					PushResult(flag33 ? "Buff 已移除" : GameApi.LastError, flag33);
					PushBuffs();
					PushSurvivalPlans();
					PushSurvivalPlanCatalog();
				}
				break;
			}
			case "getResources":
				PushResources();
				break;
			case "setExposure":
			{
				if (TryGetFloat(rootElement, "value", 0f, 100000000f, "暴露值", out var value29))
				{
					bool flag30 = GameApi.SetExposure(value29);
					PushResult(flag30 ? "暴露值已更新" : GameApi.LastError, flag30);
					PushResources();
				}
				break;
			}
			case "setNoExploreExposure":
			{
				bool flag25 = GetBool(rootElement, "on", def: false);
				bool flag26 = GameApi.SetNoExploreExposure(flag25);
				PushResult((!flag26) ? GameApi.LastError : (flag25 ? "已开启不会暴露" : "已关闭不会暴露"), flag26);
				PushResources();
				break;
			}
			case "setSurvivalPoints":
			{
				if (TryGetInt(rootElement, "value", 0, int.MaxValue, "生存点", out var value22))
				{
					bool flag21 = GameApi.SetSurvivalPoints(value22);
					PushResult(flag21 ? "生存点已更新" : GameApi.LastError, flag21);
					PushResources();
				}
				break;
			}
			case "removeBuffs":
			{
				int num = GameApi.RemoveAllNegativeBuffs();
				string lastError2 = GameApi.LastError;
				PushBuffs();
				PushSurvivalPlans();
				PushSurvivalPlanCatalog();
				GameApi.LastError = lastError2;
				bool flag17 = num >= 0 && GameApi.LastError == "";
				PushResult(flag17 ? $"已移除 {num} 个负面效果" : GameApi.LastError, flag17);
				break;
			}
			case "clearBuffs":
			{
				bool flag13 = GameApi.ClearAllBuffs();
				string lastError = GameApi.LastError;
				PushBuffs();
				PushSurvivalPlans();
				PushSurvivalPlanCatalog();
				GameApi.LastError = lastError;
				PushResult(flag13 ? "已清空普通 Buff，生存规划效果已保留" : GameApi.LastError, flag13);
				break;
			}
			case "getRelationship":
				PushRelationship();
				break;
			case "getProficiency":
				PushProficiencies();
				break;
			case "addProficiencyExp":
			{
				if (TryGetInt(rootElement, "typeId", 1, 6, "熟练度类型", out var value13) && TryGetInt(rootElement, "amount", 1, 100000000, "熟练度经验", out var value14))
				{
					bool flag10 = GameApi.AddProficiencyExp(value13, value14);
					PushResult(flag10 ? $"已增加 {value14} 熟练度经验" : GameApi.LastError, flag10);
					PushProficiencies();
				}
				break;
			}
			case "addProficiencyLevel":
			{
				if (TryGetInt(rootElement, "typeId", 1, 6, "熟练度类型", out var value6) && TryGetInt(rootElement, "levels", 1, 5, "提升等级", out var value7))
				{
					bool flag5 = GameApi.AddProficiencyLevels(value6, value7, out var applied);
					PushResult(flag5 ? $"已提升 {applied} 级熟练度" : GameApi.LastError, flag5);
					PushProficiencies();
				}
				break;
			}
			case "setRelationship":
			{
				if (TryGetInt(rootElement, "v", 0, 1000000, "好感度", out var value2))
				{
					bool flag = GameApi.SetRelationship(value2);
					PushResult(flag ? "已设置好感度" : GameApi.LastError, flag);
					PushRelationship();
				}
				break;
			}
			case "getClipboard":
			{
				string s = "";
				try
				{
					s = GUIUtility.systemCopyBuffer ?? "";
				}
				catch (Exception ex)
				{
					GameApi.LogErr("getClipboard: " + ex.Message);
				}
				PushData("{\"type\":\"clipboard\",\"text\":\"" + JsonEsc(s) + "\"}");
				break;
			}
			case "setPanelOpen":
				PanelOpen = GetBool(rootElement, "on", def: false);
				ApplyPanelState();
				break;
			default:
				GameApi.LogErr("Unknown UI command: " + text);
				break;
			}
		}
		catch (Exception ex2)
		{
			GameApi.LogErr("HandleCommand[" + text + "]: " + ex2.Message);
			PushResult("操作失败：命令数据无效", ok: false);
		}
	}

	private static void PushActiveTabSnapshot()
	{
		bool isReady = GameApi.IsReady;
		PushData("{\"type\":\"status\",\"ok\":" + (isReady ? "true" : "false") + "}");
		if (!isReady)
		{
			return;
		}
		GameApi.ApplyFrozenOverride();
		string activeTab = _activeTab;
		if (activeTab == null)
		{
			return;
		}
		switch (activeTab.Length)
		{
		case 5:
			switch (activeTab[0])
			{
			case 'm':
				if (activeTab == "money")
				{
					int num = GameApi.CurrentGold();
					if (num >= 0)
					{
						PushData("{\"type\":\"gold\",\"val\":" + num + "}");
					}
				}
				break;
			case 'i':
				if (activeTab == "items" && string.IsNullOrEmpty(_cachedItemsPayload))
				{
					PushItems();
				}
				break;
			case 'b':
				if (activeTab == "buffs")
				{
					PushBuffConfigs();
					PushSurvivalPlans();
					PushSurvivalPlanCatalog();
					PushBuffs();
				}
				break;
			}
			break;
		case 10:
			switch (activeTab[0])
			{
			case 'a':
				if (activeTab == "attributes")
				{
					PushAttrs();
					PushMoveSpeed();
					PushRelationship();
				}
				break;
			case 'f':
				if (activeTab == "facilities")
				{
					PushFacilities();
				}
				break;
			}
			break;
		case 7:
			if (activeTab == "prepare")
			{
				int num2 = GameApi.CurrentGold();
				if (num2 >= 0)
				{
					PushData("{\"type\":\"gold\",\"val\":" + num2 + "}");
				}
				PushTimeInfo();
			}
			break;
		case 3:
			if (activeTab == "bag")
			{
				PushBagInfo();
			}
			break;
		case 4:
			if (activeTab == "time")
			{
				PushTimeInfo();
			}
			break;
		case 11:
			if (activeTab == "proficiency")
			{
				PushProficiencies();
			}
			break;
		case 9:
			if (activeTab == "resources")
			{
				PushResources();
			}
			break;
		case 6:
		case 8:
			break;
		}
	}

	private static bool TryParseAttr(string name, out AttrName attr)
	{
		switch (name)
		{
		case "Health":
			attr = (AttrName)5;
			return true;
		case "Stamina":
			attr = (AttrName)3;
			return true;
		case "Satiety":
			attr = (AttrName)1;
			return true;
		case "Morale":
			attr = (AttrName)2;
			return true;
		default:
			attr = (AttrName)0;
			return false;
		}
	}

	private static bool TryParseMaxAttr(string name, out AttrName attr)
	{
		switch (name)
		{
		case "Health":
			attr = (AttrName)105;
			return true;
		case "Stamina":
			attr = (AttrName)103;
			return true;
		case "Satiety":
			attr = (AttrName)101;
			return true;
		case "Morale":
			attr = (AttrName)102;
			return true;
		default:
			attr = (AttrName)0;
			return false;
		}
	}

	private static bool TryGetInt(JsonElement root, string name, int minimum, int maximum, string label, out int value)
	{
		value = 0;
		if (!root.TryGetProperty(name, out var value2) || value2.ValueKind != JsonValueKind.Number || !value2.TryGetInt32(out value))
		{
			PushResult(label + "必须是整数", ok: false);
			return false;
		}
		if (value < minimum || value > maximum)
		{
			PushResult($"{label}范围为 {minimum} 到 {maximum}", ok: false);
			return false;
		}
		return true;
	}

	private static bool TryGetLong(JsonElement root, string name, string label, out long value)
	{
		value = 0L;
		if (!root.TryGetProperty(name, out var value2) || value2.ValueKind != JsonValueKind.Number || !value2.TryGetInt64(out value) || value <= 0)
		{
			PushResult(label + "无效", ok: false);
			return false;
		}
		return true;
	}

	private static bool TryGetFloat(JsonElement root, string name, float minimum, float maximum, string label, out float value)
	{
		value = 0f;
		if (!root.TryGetProperty(name, out var value2) || value2.ValueKind != JsonValueKind.Number || !value2.TryGetSingle(out value) || !float.IsFinite(value))
		{
			PushResult(label + "必须是有效数字", ok: false);
			return false;
		}
		if (value < minimum || value > maximum)
		{
			PushResult(label + $"范围为 {minimum:0.##} 到 {maximum:0.##}", ok: false);
			return false;
		}
		return true;
	}

	private static int GetOptionalInt(JsonElement root, string name, int fallback)
	{
		if (!root.TryGetProperty(name, out var value) || value.ValueKind != JsonValueKind.Number || !value.TryGetInt32(out var value2))
		{
			return fallback;
		}
		if (value2 >= 0)
		{
			return Math.Min(1000000, value2);
		}
		return -1;
	}

	private static bool GetBool(JsonElement e, string name, bool def)
	{
		try
		{
			return e.GetProperty(name).GetBoolean();
		}
		catch
		{
			return def;
		}
	}

	private static void PushResult(string msg, bool ok, bool withGold = false)
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"result\",\"ok\":").Append(ok ? "true" : "false").Append(",\"msg\":\"")
			.Append(JsonEsc(msg))
			.Append("\"");
		if (withGold)
		{
			int num = GameApi.CurrentGold();
			if (num >= 0)
			{
				stringBuilder.Append(",\"gold\":").Append(num);
			}
		}
		stringBuilder.Append("}");
		PushData(stringBuilder.ToString());
	}

	private static void PushItems()
	{
		try
		{
			if (!string.IsNullOrEmpty(_cachedItemsPayload))
			{
				PushData(_cachedItemsPayload);
				return;
			}
			if (!ItemCatalog.IsReady)
			{
				ItemCatalog.Refresh();
			}
			if (!ItemCatalog.IsReady)
			{
				return;
			}
			StringBuilder stringBuilder = new StringBuilder();
			stringBuilder.Append("{\"type\":\"items\",\"items\":[");
			var items = ItemCatalog.Items;
			for (int i = 0; i < items.Count; i++)
			{
				if (i > 0)
				{
					stringBuilder.Append(',');
				}
				ItemInfo itemInfo = items[i];
				stringBuilder.Append("{\"id\":").Append(itemInfo.Id).Append(",\"name\":\"")
					.Append(JsonEsc(itemInfo.Name))
					.Append("\"")
					.Append(",\"cat\":")
					.Append(itemInfo.SubCategory)
					.Append(",\"cid\":")
					.Append(itemInfo.Category)
					.Append(",\"price\":")
					.Append(itemInfo.Price)
					.Append("}");
			}
			stringBuilder.Append("],\"cats\":[");
			SortedDictionary<int, string> sortedDictionary = new SortedDictionary<int, string>();
			foreach (ItemInfo item in items)
			{
				if (item.SubCategory > 0 && !sortedDictionary.ContainsKey(item.SubCategory))
				{
					sortedDictionary[item.SubCategory] = ItemCatalog.GetSubCatName(item.SubCategory);
				}
			}
			bool flag = true;
			foreach (KeyValuePair<int, string> item2 in sortedDictionary)
			{
				if (!flag)
				{
					stringBuilder.Append(',');
				}
				flag = false;
				stringBuilder.Append("{\"id\":").Append(item2.Key).Append(",\"name\":\"")
					.Append(JsonEsc(item2.Value))
					.Append("\"}");
			}
			stringBuilder.Append("],\"tcats\":[");
			var topCategories = ItemCatalog.TopCategories;
			flag = true;
			foreach (TopCatInfo item3 in topCategories)
			{
				if (!flag)
				{
					stringBuilder.Append(',');
				}
				flag = false;
				stringBuilder.Append("{\"id\":").Append(item3.Id).Append(",\"name\":\"")
					.Append(JsonEsc(item3.Name))
					.Append("\"}");
			}
			stringBuilder.Append("]}");
			_cachedItemsPayload = stringBuilder.ToString();
			PushData(_cachedItemsPayload);
		}
		catch (Exception ex)
		{
			GameApi.LogErr("PushItems: " + ex.Message);
		}
	}

	private static void PushAttrs()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"attr\",\"source\":").Append(GameApi.GetAttributeSourceId()).Append(",\"attrs\":{");
		AppendAttr(stringBuilder, "Health", GameApi.GetAttr((AttrName)5), GameApi.GetMaxAttr((AttrName)105));
		AppendAttr(stringBuilder, "Stamina", GameApi.GetAttr((AttrName)3), GameApi.GetMaxAttr((AttrName)103));
		AppendAttr(stringBuilder, "Satiety", GameApi.GetAttr((AttrName)1), GameApi.GetMaxAttr((AttrName)101));
		AppendAttr(stringBuilder, "Morale", GameApi.GetAttr((AttrName)2), GameApi.GetMaxAttr((AttrName)102));
		stringBuilder.Append("}}");
		PushData(stringBuilder.ToString());
	}

	private static void AppendAttr(StringBuilder sb, string key, int cur, int max)
	{
		if (sb.Length > 0 && sb[sb.Length - 1] != '{')
		{
			sb.Append(',');
		}
		sb.Append('"').Append(key).Append("\":{\"cur\":")
			.Append(cur)
			.Append(",\"max\":")
			.Append(max)
			.Append('}');
	}

	private static void PushBag()
	{
		var list = GameApi.ListBackpackItems();
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"bag\",\"items\":[");
		for (int i = 0; i < list.Count; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(',');
			}
			stringBuilder.Append("{\"id\":").Append(list[i].InstanceId).Append(",\"count\":")
				.Append(list[i].Count)
				.Append(",\"name\":\"")
				.Append(JsonEsc(list[i].Name))
				.Append("\"}");
		}
		stringBuilder.Append("]}");
		PushData(stringBuilder.ToString());
	}

	private static void PushMoveSpeed()
	{
		if (!GameApi.TryGetMoveSpeed(out var current, out var original, out var multiplier))
		{
			PushData("{\"type\":\"moveSpeed\",\"ready\":false}");
			return;
		}
		PushData("{\"type\":\"moveSpeed\",\"ready\":true,\"current\":" + current.ToString("0.###", CultureInfo.InvariantCulture) + ",\"original\":" + original.ToString("0.###", CultureInfo.InvariantCulture) + ",\"multiplier\":" + multiplier.ToString("0.##", CultureInfo.InvariantCulture) + "}");
	}

	private static void PushFacilities()
	{
		GameApi.GetHomeDurabilitySummary(8, out var count, out var minimumCurrent, out var maximum);
		GameApi.GetHomeDurabilitySummary(9, out var count2, out var minimumCurrent2, out var maximum2);
		PushData("{\"type\":\"facilities\",\"doorCount\":" + count + ",\"doorCurrent\":" + minimumCurrent + ",\"doorMax\":" + maximum + ",\"windowCount\":" + count2 + ",\"windowCurrent\":" + minimumCurrent2 + ",\"windowMax\":" + maximum2 + ",\"infiniteFood\":" + (GameApi.InfiniteFoodShelfLife ? "true" : "false") + "}");
	}

	private static void PushBagInfo()
	{
		if (!BackpackManager.TryGetInfo(out var columns, out var rows, out var weight, out var maxBurden, out var baseColumns, out var baseRows, out var naturalMaxBurden))
		{
			PushData("{\"type\":\"bagInfo\",\"ready\":false}");
			return;
		}
		PushData("{\"type\":\"bagInfo\",\"ready\":true,\"columns\":" + columns + ",\"rows\":" + rows + ",\"weight\":" + weight + ",\"maxBurden\":" + maxBurden + ",\"baseColumns\":" + baseColumns + ",\"baseRows\":" + baseRows + ",\"naturalMaxBurden\":" + naturalMaxBurden + "}");
	}

	private static string JsonEsc(string s)
	{
		if (s == null)
		{
			return "";
		}
		StringBuilder stringBuilder = new StringBuilder();
		foreach (char c in s)
		{
			switch (c)
			{
			case '"':
				stringBuilder.Append("\\\"");
				continue;
			case '\\':
				stringBuilder.Append("\\\\");
				continue;
			case '\b':
				stringBuilder.Append("\\b");
				continue;
			case '\f':
				stringBuilder.Append("\\f");
				continue;
			case '\n':
				stringBuilder.Append("\\n");
				continue;
			case '\r':
				stringBuilder.Append("\\r");
				continue;
			case '\t':
				stringBuilder.Append("\\t");
				continue;
			}
			if (c < ' ')
			{
				StringBuilder stringBuilder2 = stringBuilder.Append("\\u");
				int num = c;
				stringBuilder2.Append(num.ToString("X4"));
			}
			else
			{
				stringBuilder.Append(c);
			}
		}
		return stringBuilder.ToString();
	}

	private static void PushTimeInfo()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"time\",").Append("\"day\":").Append(GameApi.GameDay())
			.Append(",\"hour\":")
			.Append(GameApi.GameHour())
			.Append(",\"totalSeconds\":")
			.Append(GameApi.GameTotalSeconds())
			.Append(",\"remainHour\":")
			.Append(GameApi.RemainCountdownHour().ToString("F1", CultureInfo.InvariantCulture))
			.Append(",\"frozen\":")
			.Append(GameApi.IsClockFrozen() ? "true" : "false")
			.Append(",\"timer\":\"")
			.Append(JsonEsc(GameApi.TimerTypeName()))
			.Append("\"")
			.Append("}");
		PushData(stringBuilder.ToString());
	}

	private static void PushRelationship()
	{
		RelationshipView relationship = GameApi.GetRelationship();
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"relationship\",").Append("\"name\":\"").Append(JsonEsc(relationship.Name))
			.Append("\"")
			.Append(",\"affinity\":")
			.Append(relationship.Affinity)
			.Append(",\"tier\":")
			.Append(relationship.Tier)
			.Append(",\"maxAffinity\":")
			.Append(relationship.MaxAffinity)
			.Append(",\"locked\":")
			.Append(relationship.Locked ? "true" : "false")
			.Append(",\"tierName\":\"")
			.Append(JsonEsc(relationship.TierName))
			.Append("\"")
			.Append("}");
		PushData(stringBuilder.ToString());
	}

	public static void TogglePanel()
	{
		PanelOpen = !PanelOpen;
		ApplyPanelState();
		PushData("{\"type\":\"panel\",\"open\":" + (PanelOpen ? "true" : "false") + "}");
	}

	private static void ApplyPanelState()
	{
		try
		{
			ReduxUISystem instance = BaseSingleton<ReduxUISystem>.Instance;
			WebUILayer val = ((instance != null) ? instance.GetWebUILayer() : null);
			if (val != null)
			{
				try
				{
					WebViewStaticMask cachedStaticMask = val.cachedStaticMask;
					if (PanelOpen)
					{
						if (_storedWebLayer != null && _storedWebLayer != val)
						{
							RestoreRaycastValidator();
							RestoreWebLayerState();
						}
						_storedWebLayer = val;
						if (!_webLayerStateStored && (object)val.canvasGroup != null)
						{
							_canvasAlpha = val.canvasGroup.alpha;
							_canvasBlocksRaycasts = val.canvasGroup.blocksRaycasts;
							_canvasInteractable = val.canvasGroup.interactable;
							_storedWebLayer = val;
							_webLayerStateStored = true;
						}
						val.ElevateToLayer((Layer)6);
						if ((object)val.canvasGroup != null)
						{
							val.canvasGroup.blocksRaycasts = true;
							val.canvasGroup.interactable = true;
							val.canvasGroup.alpha = 1f;
						}
						if ((object)cachedStaticMask != null)
						{
							if ((object)_storedStaticMask != null && (object)_storedStaticMask != (object)cachedStaticMask)
							{
								RestoreRaycastValidator();
							}
							if (!_raycastValidatorStored && cachedStaticMask.RaycastValidator != null)
							{
								_originalValidator = cachedStaticMask.RaycastValidator;
								_storedStaticMask = cachedStaticMask;
								_raycastValidatorStored = true;
								GameApi.LogInfo("[WebUiBridge] 已保存游戏原始鼠标区域回调");
							}
							if (_raycastValidatorStored && _alwaysTrueValidator == null)
							{
								_alwaysTrueValidator = DelegateSupport.ConvertDelegate<Il2CppSystem.Func<Vector2, Camera, bool>>((Delegate)new Func<Vector2, Camera, bool>(AlwaysTrueRaycast));
							}
							if (_raycastValidatorStored)
							{
								cachedStaticMask.RaycastValidator = _alwaysTrueValidator;
							}
						}
					}
				}
				catch (Exception ex)
				{
					GameApi.LogErr("ApplyPanelState(web): " + ex.Message);
				}
			}
			GameApi.SetMovementBlocked(PanelOpen);
			GameApi.SetHotKeyDisabled(PanelOpen);
			if (PanelOpen)
			{
				ManageInput();
				BoostFrameRateForPanel();
				return;
			}
			ResetDragState();
			try
			{
				if (IsUsableWebView(_web))
				{
					_web.SetFocused(false);
				}
			}
			catch
			{
			}
			_webFocusApplied = false;
			Push("window.dispatchEvent(new Event('slc-drag-cancel'));");
			RestoreWebPrefabDragMode();
			RestoreRaycastValidator();
			RestoreWebLayerState();
			RestoreFrameRateAfterPanel();
			GameApi.LogInfo("[WebUiBridge] 面板已关闭，鼠标输入层已恢复");
		}
		catch (Exception ex2)
		{
			GameApi.LogErr("ApplyPanelState: " + ex2.Message);
		}
	}

	public static void ManageInput()
	{
		//IL_008d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0093: Invalid comparison between Unknown and I4
		//IL_005a: Unknown result type (might be due to invalid IL or missing references)
		//IL_005f: Unknown result type (might be due to invalid IL or missing references)
		if (!PanelOpen)
		{
			return;
		}
		try
		{
			if (_dragModeStored && _webFocusApplied && (double)Time.unscaledTime < _nextInputEnsure)
			{
				return;
			}
			_nextInputEnsure = (double)Time.unscaledTime + 0.5;
			if ((object)_gamePrefab != null)
			{
				if (!_dragModeStored)
				{
					_originalDragMode = _gamePrefab.DragMode;
					_prefabKeyboardEnabled = _gamePrefab.KeyboardEnabled;
					_prefabClickingEnabled = _gamePrefab.ClickingEnabled;
					_dragModeStored = true;
				}
				if ((int)_gamePrefab.DragMode != 1)
				{
					_gamePrefab.DragMode = (DragMode)1;
				}
				if (!_gamePrefab.KeyboardEnabled)
				{
					_gamePrefab.KeyboardEnabled = true;
				}
				if (!_gamePrefab.ClickingEnabled)
				{
					_gamePrefab.ClickingEnabled = true;
				}
			}
			if (_web == null || _webFocusApplied)
			{
				return;
			}
			IWebView web = _web;
			if (!IsUsableWebView(web))
			{
				InvalidateWebView(web);
				return;
			}
			try
			{
				web.SetFocused(true);
				_webFocusApplied = true;
			}
			catch
			{
				InvalidateWebView(web);
			}
		}
		catch (Exception ex)
		{
			if (!_inputErrorLogged)
			{
				_inputErrorLogged = true;
				GameApi.LogErr("ManageInput: " + ex.Message);
			}
		}
	}

	public static void KeepPanelInput()
	{
		if (!PanelOpen)
		{
			return;
		}
		try
		{
			ReduxUISystem instance = BaseSingleton<ReduxUISystem>.Instance;
			WebUILayer val = ((instance != null) ? instance.GetWebUILayer() : null);
			if (val == null)
			{
				return;
			}
			WebViewStaticMask cachedStaticMask = val.cachedStaticMask;
			bool flag = _storedWebLayer != val;
			bool flag2 = (object)cachedStaticMask != (object)_storedStaticMask;
			if ((_storedWebLayer != null) & flag)
			{
				RestoreRaycastValidator();
				RestoreWebLayerState();
			}
			double num = Time.unscaledTime;
			if (!flag && !flag2 && _webLayerStateStored && num < _nextPanelInputEnsure)
			{
				return;
			}
			_nextPanelInputEnsure = num + 0.5;
			_storedWebLayer = val;
			if ((flag | flag2) || !_webLayerStateStored)
			{
				try
				{
					val.ElevateToLayer((Layer)6);
				}
				catch
				{
				}
			}
			if ((object)val.canvasGroup != null)
			{
				if (!_webLayerStateStored)
				{
					_canvasAlpha = val.canvasGroup.alpha;
					_canvasBlocksRaycasts = val.canvasGroup.blocksRaycasts;
					_canvasInteractable = val.canvasGroup.interactable;
					_webLayerStateStored = true;
				}
				if (!val.canvasGroup.blocksRaycasts)
				{
					val.canvasGroup.blocksRaycasts = true;
				}
				if (!val.canvasGroup.interactable)
				{
					val.canvasGroup.interactable = true;
				}
				if (val.canvasGroup.alpha != 1f)
				{
					val.canvasGroup.alpha = 1f;
				}
			}
			if ((object)cachedStaticMask != null)
			{
				if ((object)_storedStaticMask != null && (object)_storedStaticMask != (object)cachedStaticMask)
				{
					RestoreRaycastValidator();
				}
				if (!_raycastValidatorStored && cachedStaticMask.RaycastValidator != null)
				{
					_originalValidator = cachedStaticMask.RaycastValidator;
					_storedStaticMask = cachedStaticMask;
					_raycastValidatorStored = true;
					GameApi.LogInfo("[WebUiBridge] 已保存游戏原始鼠标区域回调");
				}
				if (_raycastValidatorStored && _alwaysTrueValidator == null)
				{
					_alwaysTrueValidator = DelegateSupport.ConvertDelegate<Il2CppSystem.Func<Vector2, Camera, bool>>((Delegate)new Func<Vector2, Camera, bool>(AlwaysTrueRaycast));
				}
				if (_raycastValidatorStored && cachedStaticMask.RaycastValidator != _alwaysTrueValidator)
				{
					cachedStaticMask.RaycastValidator = _alwaysTrueValidator;
				}
			}
		}
		catch (Exception ex)
		{
			if (!_inputErrorLogged)
			{
				_inputErrorLogged = true;
				GameApi.LogErr("KeepPanelInput: " + ex.Message);
			}
		}
	}

	private static void RestoreWebPrefabDragMode()
	{
		//IL_0019: Unknown result type (might be due to invalid IL or missing references)
		try
		{
			if ((object)_gamePrefab != null && _dragModeStored)
			{
				_gamePrefab.DragMode = _originalDragMode;
				_gamePrefab.KeyboardEnabled = _prefabKeyboardEnabled;
				_gamePrefab.ClickingEnabled = _prefabClickingEnabled;
			}
		}
		catch
		{
		}
		_dragModeStored = false;
	}

	private static void PushSurvivalPlans()
	{
		var survivalPlans = GameApi.GetSurvivalPlans();
		bool flag = GameApi.IsReady && (survivalPlans.Count > 0 || string.IsNullOrEmpty(GameApi.LastError));
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"survivalPlans\",\"ready\":").Append(flag ? "true" : "false").Append(",\"items\":[");
		for (int i = 0; i < survivalPlans.Count; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(',');
			}
			SurvivalPlanView survivalPlanView = survivalPlans[i];
			stringBuilder.Append("{\"talentId\":").Append(survivalPlanView.TalentId).Append(",\"name\":\"")
				.Append(JsonEsc(survivalPlanView.Name))
				.Append("\"")
				.Append(",\"description\":\"")
				.Append(JsonEsc(survivalPlanView.Description))
				.Append("\"")
				.Append(",\"level\":")
				.Append(survivalPlanView.Level)
				.Append(",\"active\":")
				.Append(survivalPlanView.Active ? "true" : "false")
				.Append('}');
		}
		stringBuilder.Append("]}");
		PushData(stringBuilder.ToString());
	}

	private static void PushSurvivalPlanCatalog()
	{
		var survivalPlanCatalog = GameApi.GetSurvivalPlanCatalog();
		bool flag = GameApi.IsReady && (survivalPlanCatalog.Count > 0 || string.IsNullOrEmpty(GameApi.LastError));
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"survivalPlanCatalog\",\"ready\":").Append(flag ? "true" : "false").Append(",\"items\":[");
		for (int i = 0; i < survivalPlanCatalog.Count; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(',');
			}
			SurvivalPlanView survivalPlanView = survivalPlanCatalog[i];
			stringBuilder.Append("{\"talentId\":").Append(survivalPlanView.TalentId).Append(",\"name\":\"")
				.Append(JsonEsc(survivalPlanView.Name))
				.Append("\"")
				.Append(",\"description\":\"")
				.Append(JsonEsc(survivalPlanView.Description))
				.Append("\"")
				.Append(",\"level\":")
				.Append(survivalPlanView.Level)
				.Append(",\"active\":")
				.Append(survivalPlanView.Active ? "true" : "false")
				.Append('}');
		}
		stringBuilder.Append("]}");
		PushData(stringBuilder.ToString());
	}

	private static void PushBuffConfigs()
	{
		var buffConfigs = GameApi.GetBuffConfigs();
		bool flag = buffConfigs.Count > 0 || string.IsNullOrEmpty(GameApi.LastError);
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"buffConfigs\",\"ready\":").Append(flag ? "true" : "false").Append(",\"items\":[");
		for (int i = 0; i < buffConfigs.Count; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(',');
			}
			BuffConfigView buffConfigView = buffConfigs[i];
			stringBuilder.Append("{\"configId\":").Append(buffConfigView.ConfigId).Append(",\"name\":\"")
				.Append(JsonEsc(buffConfigView.Name))
				.Append("\"")
				.Append(",\"good\":")
				.Append(buffConfigView.IsGood ? "true" : "false")
				.Append(",\"duration\":")
				.Append(buffConfigView.Duration.ToString(CultureInfo.InvariantCulture))
				.Append('}');
		}
		stringBuilder.Append("]}");
		PushData(stringBuilder.ToString());
	}

	private static void PushBuffs()
	{
		var buffs = GameApi.GetBuffs();
		bool flag = GameApi.IsReady && (buffs.Count > 0 || string.IsNullOrEmpty(GameApi.LastError));
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"buffs\",\"ready\":").Append(flag ? "true" : "false").Append(",\"items\":[");
		for (int i = 0; i < buffs.Count; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(',');
			}
			BuffView buffView = buffs[i];
			stringBuilder.Append("{\"instanceId\":").Append(buffView.InstanceId).Append(",\"configId\":")
				.Append(buffView.ConfigId)
				.Append(",\"name\":\"")
				.Append(JsonEsc(buffView.Name))
				.Append("\"")
				.Append(",\"good\":")
				.Append(buffView.IsGood ? "true" : "false")
				.Append(",\"layers\":")
				.Append(buffView.Layers)
				.Append(",\"timeEndTime\":")
				.Append(buffView.TimeEndTime)
				.Append(",\"removeByConfig\":")
				.Append(buffView.RemoveByConfig ? "true" : "false")
				.Append(",\"source\":\"")
				.Append(JsonEsc(buffView.Source))
				.Append("\"}");
		}
		stringBuilder.Append("]}");
		PushData(stringBuilder.ToString());
	}

	private static void PushResources()
	{
		bool flag = GameApi.TryGetExposure(out var current, out var maximum, out var timeExposure, out var moveExposure, out var running);
		int num = GameApi.CurrentSurvivalPoints();
		bool flag2 = (GameApi.IsReady & flag) && num >= 0;
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"resources\",\"ready\":").Append(flag2 ? "true" : "false").Append(",\"exposure\":")
			.Append(current.ToString(CultureInfo.InvariantCulture))
			.Append(",\"maxExposure\":")
			.Append(maximum)
			.Append(",\"timeExposure\":")
			.Append(timeExposure.ToString(CultureInfo.InvariantCulture))
			.Append(",\"moveExposure\":")
			.Append(moveExposure.ToString(CultureInfo.InvariantCulture))
			.Append(",\"running\":")
			.Append(running ? "true" : "false")
			.Append(",\"noExposure\":")
			.Append(GameApi.NoExploreExposure ? "true" : "false")
			.Append(",\"survivalPoints\":")
			.Append(num)
			.Append('}');
		PushData(stringBuilder.ToString());
	}

	private static void PushProficiencies()
	{
		var proficiencies = GameApi.GetProficiencies();
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("{\"type\":\"proficiency\",\"ready\":").Append((proficiencies.Count > 0) ? "true" : "false").Append(",\"items\":[");
		for (int i = 0; i < proficiencies.Count; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(',');
			}
			ProficiencyView proficiencyView = proficiencies[i];
			stringBuilder.Append("{\"typeId\":").Append(proficiencyView.TypeId).Append(",\"name\":\"")
				.Append(JsonEsc(proficiencyView.Name))
				.Append("\"")
				.Append(",\"level\":")
				.Append(proficiencyView.Level)
				.Append(",\"exp\":")
				.Append(proficiencyView.Exp)
				.Append(",\"previousLevelExp\":")
				.Append(proficiencyView.PreviousLevelExp)
				.Append(",\"nextLevelExp\":")
				.Append(proficiencyView.NextLevelExp)
				.Append(",\"maxLevel\":")
				.Append(proficiencyView.MaxLevel)
				.Append('}');
		}
		stringBuilder.Append("]}");
		PushData(stringBuilder.ToString());
	}

	public static void Shutdown()
	{
		StopHeartbeat();
		PanelOpen = false;
		_panelCloseRequested = false;
		ResetDragState();
		_commandWakeRequested = false;
		_commandReadPending = false;
		_pending.Clear();
		try
		{
			Push("window.dispatchEvent(new Event('slc-drag-cancel'));");
		}
		catch
		{
		}
		UnsubscribeWebMessages();
		RestoreWebPrefabDragMode();
		RestoreRaycastValidator();
		RestoreWebLayerState();
		RestoreFrameRateAfterPanel();
		GameApi.SetMovementBlocked(blocked: false);
		GameApi.SetHotKeyDisabled(disabled: false);
		_webFocusApplied = false;
		_web = null;
		_gamePrefab = null;
	}
}
