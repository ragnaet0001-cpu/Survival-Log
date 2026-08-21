using System.Collections.Generic;
using System.Reflection;
using GameCore.HotUpdate;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch]
public static class InfiniteFoodShelfLifeTextPatch
{
	[HarmonyTargetMethods]
	public static IEnumerable<MethodBase> TargetMethods()
	{
		return InfiniteFoodReduxTargets.FindMethods("GetShelfLifeText");
	}

	[HarmonyPrefix]
	public static bool Prefix(Config_Item config, ref string __result)
	{
		if (!GameApi.InfiniteFoodShelfLife || config == null || config.Life <= 0)
		{
			return true;
		}
		__result = "∞";
		return false;
	}
}
