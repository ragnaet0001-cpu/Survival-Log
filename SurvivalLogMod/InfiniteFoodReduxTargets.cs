using System;
using System.Collections.Generic;
using System.Reflection;
using GameCore.HotUpdate.ReduxUI;
using HarmonyLib;

namespace SurvivalLogCheat;

internal static class InfiniteFoodReduxTargets
{
	internal static readonly Type[] Types = new Type[6]
	{
		typeof(Reducer_Web_BackpackUI),
		typeof(Reducer_Web_Brew),
		typeof(Reducer_Web_Cooking),
		typeof(Reducer_Web_RatCage),
		typeof(Reducer_Web_ShopUI),
		typeof(Reducer_Web_TradeUI)
	};

	internal static IEnumerable<MethodBase> FindMethods(string methodName)
	{
		Type[] types = Types;
		for (int i = 0; i < types.Length; i++)
		{
			MethodInfo methodInfo = AccessTools.Method(types[i], methodName, (Type[])null, (Type[])null);
			if (methodInfo != null)
			{
				yield return methodInfo;
			}
		}
	}
}
