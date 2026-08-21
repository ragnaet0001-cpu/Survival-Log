using GameCore.HotUpdate.Battle.Logic;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(GameTimeManager), "CostTime")]
public static class CostTimePatch
{
	public static bool Prefix(GameTimeManager __instance)
	{
		if (__instance.IsClockFrozen || GameApi.FrozenOverride)
		{
			TimeFreezePatches.ShouldBlock("GameTimeManager.CostTime");
			return false;
		}
		return true;
	}
}
