using GameCore.HotUpdate;
using GameCore.HotUpdate.ReduxUI;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(WebUILayer), "GetTargetWebFrameRateForTier")]
public static class WebFrameRatePatch
{
	private static bool _logged;

	public static bool Prefix(ref uint __result)
	{
		int num = 0;
		try
		{
			num = Plugin.WebViewFrameRateConfig?.Value ?? 0;
		}
		catch
		{
		}
		if (num == 0)
		{
			try
			{
				SettingManager instance = BaseSingleton<SettingManager>.Instance;
				int num2 = ((instance != null) ? instance.GetTargetFrameRate() : 0);
				num = ((num2 > 0) ? num2 : 240);
			}
			catch
			{
				num = 240;
			}
		}
		if (num < 30)
		{
			num = 30;
		}
		if (num > 240)
		{
			num = 240;
		}
		__result = (uint)num;
		if (!_logged)
		{
			_logged = true;
			GameApi.LogInfo("[WebView] Chromium target frame rate set to " + num + " FPS.");
		}
		return false;
	}
}
