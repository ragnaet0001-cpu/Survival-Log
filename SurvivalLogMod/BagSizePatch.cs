using System;
using GameCore.HotUpdate.Battle.Logic;
using HarmonyLib;
using UnityEngine;

namespace SurvivalLogCheat;

[HarmonyPatch(typeof(ItemManager), "GetOwnerBagSize", new Type[] { typeof(long) })]
internal static class BagSizePatch
{
	public static void Postfix(long ownerId, ref Vector2Int __result)
	{
		BackpackManager.ApplySizeOverride(ownerId, ref __result);
	}
}
