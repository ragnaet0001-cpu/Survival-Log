using System.Collections.Generic;
using System.Reflection;
using HarmonyLib;

namespace SurvivalLogCheat;

[HarmonyPatch]
public static class InfiniteFoodExpiredStatePatch
{
	[HarmonyTargetMethods]
	public static IEnumerable<MethodBase> TargetMethods()
	{
		return InfiniteFoodReduxTargets.FindMethods("IsItemExpired");
	}

	[HarmonyPrefix]
	public static bool Prefix(ref bool __result)
	{
		if (!GameApi.InfiniteFoodShelfLife)
		{
			return true;
		}
		__result = false;
		return false;
	}
}
