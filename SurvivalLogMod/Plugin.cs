using System;
using BepInEx;
using BepInEx.Configuration;
using BepInEx.Core.Logging.Interpolation;
using BepInEx.Logging;
using BepInEx.Unity.IL2CPP;
using HarmonyLib;
using Il2CppInterop.Runtime.Injection;
using UnityEngine;

namespace SurvivalLogCheat;

[BepInPlugin("cn.miaopasi.survivallog", "SurvivalLogMod", "2.1.0")]
public class Plugin : BasePlugin
{
	public static ManualLogSource LogSource;

	public static ConfigEntry<string> ToggleHotkeysConfig;

	public static ConfigEntry<int> WebViewFrameRateConfig;

	public override void Load()
	{
		//IL_0063: Unknown result type (might be due to invalid IL or missing references)
		//IL_0069: Expected O, but got Unknown
		//IL_013c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0143: Expected O, but got Unknown
		//IL_0194: Unknown result type (might be due to invalid IL or missing references)
		//IL_019b: Expected O, but got Unknown
		//IL_01e6: Unknown result type (might be due to invalid IL or missing references)
		//IL_01eb: Unknown result type (might be due to invalid IL or missing references)
		//IL_01f1: Expected O, but got Unknown
		LogSource = ((BasePlugin)this).Log;
		ToggleHotkeysConfig = ((BasePlugin)this).Config.Bind<string>("界面", "呼出热键", "Insert,F6,F7,F8", "用逗号分隔的 Unity KeyCode 名称；修改后重启游戏生效。");
		WebViewFrameRateConfig = ((BasePlugin)this).Config.Bind<int>("Interface", "WebViewFrameRate", 0, "WebView 目标帧率；0 表示跟随游戏帧数，固定值范围 30-240。当前 WebView 已初始化时将在下次启动生效。");
		CheatGUI.ConfigureToggleHotkeys(ToggleHotkeysConfig.Value);
		Harmony val = new Harmony("cn.slcheat.survivallog.harmony");
		Type[] array = new Type[12]
		{
			typeof(BagSizePatch),
			typeof(WebFrameRatePatch),
			typeof(RenderFreezeFrameRatePatch),
			typeof(InfiniteFoodRotPatch),
			typeof(InfiniteFoodShelfLifeTextPatch),
			typeof(InfiniteFoodExpiredStatePatch),
			typeof(InfiniteFoodShelfLifeDaysPatch),
			typeof(CostTimePatch),
			typeof(CostTimeCountUpPatch),
			typeof(TimeUpdatePatch),
			typeof(CostTimeCountDownPatch),
			typeof(CostHourPatch)
		};
		int num = 0;
		bool flag = default(bool);
		foreach (Type type in array)
		{
			try
			{
				val.CreateClassProcessor(type).Patch();
				num++;
			}
			catch (Exception ex)
			{
				ManualLogSource log = ((BasePlugin)this).Log;
				BepInExErrorLogInterpolatedStringHandler val2 = new BepInExErrorLogInterpolatedStringHandler(26, 2, out flag);
				if (flag)
				{
					((BepInExLogInterpolatedStringHandler)val2).AppendLiteral("[Harmony] ");
					((BepInExLogInterpolatedStringHandler)val2).AppendFormatted<string>(type.Name);
					((BepInExLogInterpolatedStringHandler)val2).AppendLiteral(" 补丁失败，其他功能继续加载: ");
					((BepInExLogInterpolatedStringHandler)val2).AppendFormatted<Exception>(ex);
				}
				log.LogError(val2);
			}
		}
		ManualLogSource log2 = ((BasePlugin)this).Log;
		BepInExInfoLogInterpolatedStringHandler val3 = new BepInExInfoLogInterpolatedStringHandler(21, 2, out flag);
		if (flag)
		{
			((BepInExLogInterpolatedStringHandler)val3).AppendLiteral("[Harmony] 已应用 ");
			((BepInExLogInterpolatedStringHandler)val3).AppendFormatted<int>(num);
			((BepInExLogInterpolatedStringHandler)val3).AppendLiteral("/");
			((BepInExLogInterpolatedStringHandler)val3).AppendFormatted<int>(array.Length);
			((BepInExLogInterpolatedStringHandler)val3).AppendLiteral(" 个兼容补丁");
		}
		log2.LogInfo(val3);
		try
		{
			ClassInjector.RegisterTypeInIl2Cpp<CheatGUI>();
			GameObject val4 = new GameObject("SurvivalLogCheat");
			UnityEngine.Object.DontDestroyOnLoad((UnityEngine.Object)val4);
			val4.AddComponent<CheatGUI>();
			((BasePlugin)this).Log.LogInfo((object)"=== SurvivalLogCheat loaded ===");
		}
		catch (Exception ex2)
		{
			((BasePlugin)this).Log.LogError((object)("SurvivalLogCheat load failed: " + ex2));
		}
	}
}
