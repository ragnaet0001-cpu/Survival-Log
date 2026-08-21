using GameCore.HotUpdate.Battle.Logic;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(CountUpTimer), "CostTime")]
public static class CostTimeCountUpPatch
{
	public static bool Prefix()
	{
		if (TimeFreezePatches.ShouldBlock("CountUpTimer.CostTime"))
		{
			return false;
		}
		return true;
	}
}
