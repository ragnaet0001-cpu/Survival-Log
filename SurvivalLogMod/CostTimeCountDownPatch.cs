using GameCore.HotUpdate.Battle.Logic;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(CountDownTimer), "CostTime")]
public static class CostTimeCountDownPatch
{
	public static bool Prefix()
	{
		if (TimeFreezePatches.ShouldBlock("CountDownTimer.CostTime"))
		{
			return false;
		}
		return true;
	}
}
