using System;
using GameCore.HotUpdate.Battle.Logic;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(GameTimeManager), "Update", new Type[]
{
	typeof(float),
	typeof(float)
})]
internal static class TimeUpdatePatch
{
	public static void Prefix(GameTimeManager __instance)
	{
		if (!GameApi.FrozenOverride)
		{
			return;
		}
		try
		{
			if (!__instance.IsClockFrozen)
			{
				__instance.IsClockFrozen = true;
			}
		}
		catch
		{
		}
	}
}
