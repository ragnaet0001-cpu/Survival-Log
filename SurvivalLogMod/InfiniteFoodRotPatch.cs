using GameCore.HotUpdate;
using GameCore.HotUpdate.Battle.Logic;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(ItemManager), "CheckAndProcessRot")]
public static class InfiniteFoodRotPatch
{
	[HarmonyPrefix]
	public static bool Prefix(ItemData item, Config_Item config, int currentHour)
	{
		if (!GameApi.InfiniteFoodShelfLife || item == null || config == null)
		{
			return true;
		}
		if (config.Life <= 0)
		{
			return true;
		}
		item.StartTime = currentHour;
		return false;
	}
}
