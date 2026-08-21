using GameCore.HotUpdate.ReduxUI;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(Reducer_Web_BackpackUI), "GetShelfLifeDaysRaw")]
public static class InfiniteFoodShelfLifeDaysPatch
{
	[HarmonyPrefix]
	public static bool Prefix(int configLife, ref float __result)
	{
		if (!GameApi.InfiniteFoodShelfLife || configLife <= 0)
		{
			return true;
		}
		__result = (float)configLife / 24f;
		return false;
	}
}
