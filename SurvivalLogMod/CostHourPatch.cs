using GameCore.HotUpdate.ReduxUI;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(Ac_Player_UpdateCostHour), "SendAction")]
public static class CostHourPatch
{
	public static bool Prefix()
	{
		if (TimeFreezePatches.ShouldBlock("Ac_Player_UpdateCostHour.SendAction"))
		{
			return false;
		}
		return true;
	}
}
