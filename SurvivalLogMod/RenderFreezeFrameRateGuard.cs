using GameCore.HotUpdate;
using UnityEngine;

namespace SurvivalLogCheat;

public static class RenderFreezeFrameRateGuard
{
	private static int _lastLoggedRate;

	public static void Update()
	{
		SettingManager instance = BaseSingleton<SettingManager>.Instance;
		if (instance == null)
		{
			_lastLoggedRate = 0;
			return;
		}
		if (!instance.renderFreezeClampActive)
		{
			_lastLoggedRate = 0;
			return;
		}
		int targetFrameRate = instance.GetTargetFrameRate();
		if (targetFrameRate > 0 && Application.targetFrameRate != targetFrameRate)
		{
			Application.targetFrameRate = targetFrameRate;
			if (_lastLoggedRate != targetFrameRate)
			{
				_lastLoggedRate = targetFrameRate;
				GameApi.LogInfo("[FrameRateGuard] render-freeze frame rate restored to " + targetFrameRate + " FPS.");
			}
		}
	}

	public static void Restore()
	{
		_lastLoggedRate = 0;
	}
}
