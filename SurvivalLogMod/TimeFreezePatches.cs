using System.Collections.Generic;

namespace SurvivalLogCheat;

public static class TimeFreezePatches
{
	private static readonly HashSet<string> _blockLogged = new HashSet<string>();

	internal static bool ShouldBlock(string who)
	{
		if (!GameApi.FrozenOverride)
		{
			return false;
		}
		lock (_blockLogged)
		{
			if (_blockLogged.Add(who))
			{
				GameApi.LogInfo("[时间锁] 拦截 " + who + "（冻结生效，赶路/切场景不扣时）");
			}
		}
		return true;
	}
}
