using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UnityEngine;

namespace SurvivalLogCheat;

public class CheatGUI : MonoBehaviour
{
	public static CheatGUI Instance;

	private float _probeTimer;

	private float _nextLockUpdate;

	private float _nextFrozenOverride;

	private float _nextExposureUpdate;

	private float _nextInputMaintenance;

	private float _nextUpdateErrorLog;

	private string _lastUpdateError;

	private bool _worldLogged;

	private static KeyCode[] _toggleHotkeys;

	public unsafe static void ConfigureToggleHotkeys(string value)
	{
		//IL_003d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0041: Unknown result type (might be due to invalid IL or missing references)
		//IL_004a: Unknown result type (might be due to invalid IL or missing references)
		List<KeyCode> list = new List<KeyCode>();
		if (!string.IsNullOrWhiteSpace(value))
		{
			string[] array = value.Split(new char[6] { ',', '，', ';', '；', ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
			for (int i = 0; i < array.Length; i++)
			{
				if (Enum.TryParse<KeyCode>(array[i].Trim(), true, out KeyCode result) && (int)result != 0 && !list.Contains(result))
				{
					list.Add(result);
				}
			}
		}
		_toggleHotkeys = (KeyCode[])((list.Count > 0) ? ((Array)list.ToArray()) : ((Array)new KeyCode[1] { (KeyCode)277 }));
		GameApi.LogInfo("[CheatGUI] 呼出热键: " + string.Join(", ", Array.ConvertAll(_toggleHotkeys, (KeyCode key) => ((object)(*(KeyCode*)(&key))/*cast due to constrained. prefix*/).ToString())));
	}

	private void Awake()
	{
		Instance = this;
	}

	private void Start()
	{
		GameApi.LogInfo("[CheatGUI] Start - 作弊面板将注入游戏网页");
		WebUiBridge.StartHeartbeat();
		WebUiBridge.AutoOpenPanel();
	}

	private void OnDestroy()
	{
		RenderFreezeFrameRateGuard.Restore();
		WebUiBridge.Shutdown();
	}

	private void LateUpdate()
	{
		try
		{
			RenderFreezeFrameRateGuard.Update();
		}
		catch
		{
		}
	}

	private void Update()
	{
		try
		{
			UpdateSafe();
		}
		catch (Exception ex)
		{
			string text = ex.GetType().Name + ": " + ex.Message;
			if (text != _lastUpdateError || Time.unscaledTime >= _nextUpdateErrorLog)
			{
				_lastUpdateError = text;
				_nextUpdateErrorLog = Time.unscaledTime + 5f;
				GameApi.LogErr("[CheatGUI] 已隔离主循环异常: " + text);
			}
		}
	}

	private void UpdateSafe()
	{
		_probeTimer += Time.unscaledDeltaTime;
		if (!_worldLogged && _probeTimer > 2f)
		{
			_probeTimer = 0f;
			if (GameApi.World != null)
			{
				_worldLogged = true;
				try
				{
					GameApi.LogInfo(GameApi.Probe());
				}
				catch (Exception ex)
				{
					GameApi.LogErr("Probe: " + ex.Message);
				}
			}
		}
		for (int i = 0; i < _toggleHotkeys.Length; i++)
		{
			if (Input.GetKeyDown(_toggleHotkeys[i]))
			{
				WebUiBridge.TogglePanel();
				break;
			}
		}
		WebUiBridge.Update();
		if (!WebUiBridge.DragActive && Time.unscaledTime >= _nextInputMaintenance)
		{
			_nextInputMaintenance = Time.unscaledTime + 0.1f;
			WebUiBridge.ManageInput();
			WebUiBridge.KeepPanelInput();
		}
		WebUiBridge.TryApplyRememberedState();
		if (!WebUiBridge.DragActive && WebUiBridge.AnyLock && Time.unscaledTime >= _nextLockUpdate)
		{
			_nextLockUpdate = Time.unscaledTime + 0.25f;
			GameApi.ApplyAttrLocks(WebUiBridge.LockHp, WebUiBridge.LockStamina, WebUiBridge.LockSatiety, WebUiBridge.LockMorale);
		}
		if (!WebUiBridge.DragActive && GameApi.NoExploreExposure && Time.unscaledTime >= _nextExposureUpdate)
		{
			_nextExposureUpdate = Time.unscaledTime + 0.25f;
			GameApi.ApplyNoExploreExposure();
		}
		if (!WebUiBridge.DragActive && GameApi.FrozenOverride && Time.unscaledTime >= _nextFrozenOverride)
		{
			_nextFrozenOverride = Time.unscaledTime + 1f;
			GameApi.ApplyFrozenOverride();
		}
	}

	static CheatGUI()
	{
		_toggleHotkeys = new KeyCode[] { KeyCode.Insert, KeyCode.F6, KeyCode.F7, KeyCode.F8 };
	}
}
