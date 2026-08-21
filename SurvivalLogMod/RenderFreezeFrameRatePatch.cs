using GameCore.HotUpdate;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(SettingManager), "GetEffectiveTargetFrameRate")]
public static class RenderFreezeFrameRatePatch
{
	public static void Postfix(SettingManager __instance, ref int __result)
	{
		if (__instance != null && __instance.renderFreezeClampActive)
		{
			int targetFrameRate = __instance.GetTargetFrameRate();
			if (targetFrameRate > __result)
			{
				__result = targetFrameRate;
			}
		}
	}
}
