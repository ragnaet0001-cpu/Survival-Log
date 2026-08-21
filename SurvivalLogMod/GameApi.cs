using System;
using System.Text;
using BepInEx.Logging;
using GameCore.HotUpdate;
using GameCore.HotUpdate.Battle.Logic;
using GameCore.HotUpdate.Battle.Show;
using Il2CppInterop.Runtime;
using Il2CppInterop.Runtime.InteropTypes;
using Il2CppSystem.Collections.Generic;
using UnityEngine;

namespace SurvivalLogCheat;

public static class GameApi
{
	private static IntPtr _moveSpeedOwnerPointer = IntPtr.Zero;

	private static float _originalMoveSpeedBase = -1f;

	private static float _moveSpeedMultiplier = 1f;

	private static bool _noExposureErrorLogged;

	public static bool InfiniteFoodShelfLife;

	public static bool NoExploreExposure;

	public static string LastError = "";

	public static int LastAddedCount;

	public static bool LastAddedCapped;

	public static int LastGoldAdded;

	public static bool LastGoldCapped;

	public static bool FrozenOverride;

	public static BattleLogicWorld World
	{
		get
		{
			try
			{
				return TelemetryProbe.TryGetLiveWorld();
			}
			catch (Exception ex)
			{
				LogErr("TryGetLiveWorld: " + ex.Message);
				return null;
			}
		}
	}

	public static ItemManager Items
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._ItemManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Items: " + ex.Message);
				return null;
			}
		}
	}

	public static AgentManager Agents
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._AgentManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Agents: " + ex.Message);
				return null;
			}
		}
	}

	public static long PlayerId
	{
		get
		{
			try
			{
				AgentManager agents = Agents;
				return (agents != null) ? agents.GetLeadingRoleId() : 0;
			}
			catch (Exception ex)
			{
				LogErr("PlayerId: " + ex.Message);
				return 0L;
			}
		}
	}

	public static LeadingRole Player
	{
		get
		{
			try
			{
				AgentManager agents = Agents;
				return (agents != null) ? agents.GetLeadingRole() : null;
			}
			catch (Exception ex)
			{
				LogErr("Player: " + ex.Message);
				return null;
			}
		}
	}

	public static bool IsReady
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				if (world == null)
				{
					return false;
				}
				if (world._ItemManager == null)
				{
					return false;
				}
				AgentManager agentManager = world._AgentManager;
				return agentManager != null && agentManager.GetLeadingRole() != null;
			}
			catch
			{
				return false;
			}
		}
	}

	public static UserComponent PlayerUserComponent
	{
		get
		{
			try
			{
				LeadingRole player = Player;
				return (player != null) ? AgentTools.GetAgentComponent<UserComponent>((BaseAgent)(object)player) : null;
			}
			catch (Exception ex)
			{
				LogErr("UserComponent: " + ex.Message);
				return null;
			}
		}
	}

	public static AttributeComponent PlayerAttributeComponent
	{
		get
		{
			try
			{
				LeadingRole player = Player;
				return (player != null) ? AgentTools.GetAgentComponent<AttributeComponent>((BaseAgent)(object)player) : null;
			}
			catch (Exception ex)
			{
				LogErr("AttributeComponent: " + ex.Message);
				return null;
			}
		}
	}

	public static GameTimeManager Time
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._GameTimeManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Time: " + ex.Message);
				return null;
			}
		}
	}

	public static BuffComponent PlayerBuff
	{
		get
		{
			try
			{
				LeadingRole player = Player;
				return (player != null) ? AgentTools.GetAgentComponent<BuffComponent>((BaseAgent)(object)player) : null;
			}
			catch (Exception ex)
			{
				LogErr("PlayerBuff: " + ex.Message);
				return null;
			}
		}
	}

	public static AchievementManager Achievements
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._AchievementManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Achievements: " + ex.Message);
				return null;
			}
		}
	}

	public static CodexManager Codex
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._CodexManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Codex: " + ex.Message);
				return null;
			}
		}
	}

	public static SurvivalPlanningComponent PlayerSurvivalPlanning
	{
		get
		{
			try
			{
				LeadingRole player = Player;
				return (player != null) ? AgentTools.GetAgentComponent<SurvivalPlanningComponent>((BaseAgent)(object)player) : null;
			}
			catch (Exception ex)
			{
				LogErr("PlayerSurvivalPlanning: " + ex.Message);
				return null;
			}
		}
	}

	public static NeighborRescueManager Neighbor
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._NeighborRescueManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Neighbor: " + ex.Message);
				return null;
			}
		}
	}

	public static ProficiencyManager Proficiency
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._ProficiencyManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Proficiency: " + ex.Message);
				return null;
			}
		}
	}

	public static ExploreManager Explore
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._ExploreManager : null;
			}
			catch (Exception ex)
			{
				LogErr("Explore: " + ex.Message);
				return null;
			}
		}
	}

	public static SurvivalResultsManager SurvivalResults
	{
		get
		{
			try
			{
				BattleLogicWorld world = World;
				return (world != null) ? world._SurvivalResultsManager : null;
			}
			catch (Exception ex)
			{
				LogErr("SurvivalResults: " + ex.Message);
				return null;
			}
		}
	}

	public static void LogInfo(string msg)
	{
		ManualLogSource logSource = Plugin.LogSource;
		if (logSource != null)
		{
			logSource.LogInfo((object)msg);
		}
	}

	public static void LogErr(string msg)
	{
		ManualLogSource logSource = Plugin.LogSource;
		if (logSource != null)
		{
			logSource.LogError((object)msg);
		}
	}

	public static bool AddGold(int amount)
	{
		WebUiBridge.SetOp("GameApi.AddGold");
		try
		{
			LeadingRole player = Player;
			UserComponent playerUserComponent = PlayerUserComponent;
			if (player == null || playerUserComponent == null)
			{
				LastError = "未找到主角（可能不在游戏中）";
				return false;
			}
			LastGoldAdded = 0;
			LastGoldCapped = false;
			if (playerUserComponent.Money >= int.MaxValue)
			{
				LastError = "金币已达到上限";
				return false;
			}
			LastGoldAdded = (int)Math.Min(amount, 2147483647L - (long)playerUserComponent.Money);
			LastGoldCapped = LastGoldAdded != amount;
			player.AddGold(LastGoldAdded);
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "AddGold 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool SetMoney(int amount)
	{
		WebUiBridge.SetOp("GameApi.SetMoney");
		try
		{
			LeadingRole player = Player;
			UserComponent playerUserComponent = PlayerUserComponent;
			if (player == null || playerUserComponent == null)
			{
				LastError = "未找到主角/金钱组件";
				return false;
			}
			player.SyncGold(amount);
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "SetMoney 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static int CurrentGold()
	{
		try
		{
			UserComponent playerUserComponent = PlayerUserComponent;
			return (playerUserComponent != null) ? playerUserComponent.Money : (-1);
		}
		catch
		{
			return -1;
		}
	}

	public static bool AddItem(int configId, int count)
	{
		WebUiBridge.SetOp("GameApi.AddItem");
		try
		{
			bool result = BackpackManager.AddItem(configId, count, out LastAddedCount, out LastAddedCapped, out var error);
			LastError = error ?? "";
			return result;
		}
		catch (Exception ex)
		{
			LastError = "AddItem 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static System.Collections.Generic.List<BackpackItemView> ListBackpackItems()
	{
		System.Collections.Generic.List<BackpackItemView> list = new System.Collections.Generic.List<BackpackItemView>();
		try
		{
			ItemManager items = Items;
			if (items == null)
			{
				return list;
			}
			long playerId = PlayerId;
			if (playerId == 0L)
			{
				return list;
			}
			List<ItemData> itemDataList = items.GetItemDataList(playerId);
			if (itemDataList == null)
			{
				return list;
			}
			Il2CppSystem.Collections.Generic.List<ItemData>.Enumerator enumerator = itemDataList.GetEnumerator();
			while (enumerator.MoveNext())
			{
				ItemData current = enumerator.Current;
				if (current != null)
				{
					list.Add(new BackpackItemView
					{
						InstanceId = ((BaseEntity)current).InstanceId,
						ConfigId = current.ItemConfigId,
						Count = current.ItemCount,
						TimeScale = current.TimeScale
					});
				}
			}
		}
		catch (Exception ex)
		{
			LogErr("ListBackpackItems: " + ex.Message);
		}
		return list;
	}

	public static bool SetBackpackItemCount(long instanceId, int newCount)
	{
		WebUiBridge.SetOp("GameApi.SetBackpackItemCount");
		try
		{
			ItemManager items = Items;
			if (items == null)
			{
				LastError = "物品管理器不可用";
				return false;
			}
			ItemData itemData = items.GetItemData(instanceId);
			if (itemData == null)
			{
				LastError = "未找到该物品";
				return false;
			}
			ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
			Config_Item val = ((instance != null) ? instance.Get_Config_Item(itemData.ItemConfigId) : null);
			if (val == null)
			{
				LastError = "物品配置尚未加载";
				return false;
			}
			int num = ((val.StackLimit > 0) ? val.StackLimit : 9999);
			int num2 = Math.Min(newCount, num);
			items.SetItemCount(itemData, num2);
			int num3 = newCount - num2;
			if (num3 > 0 && (!BackpackManager.AddItem(itemData.ItemConfigId, num3, out var added, out var _, out var error) || added < num3))
			{
				LastError = $"单组上限为 {num}，已按合法堆叠设置 {num2 + added}/{newCount} 个";
				if (!string.IsNullOrEmpty(error))
				{
					LastError = LastError + "；" + error;
				}
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "SetCount 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool DuplicateBackpackItem(long instanceId)
	{
		WebUiBridge.SetOp("GameApi.DuplicateBackpackItem");
		try
		{
			ItemManager items = Items;
			if (items == null)
			{
				LastError = "物品管理器不可用";
				return false;
			}
			ItemData itemData = items.GetItemData(instanceId);
			if (itemData == null)
			{
				LastError = "未找到该物品";
				return false;
			}
			int itemCount = itemData.ItemCount;
			if (!AddItem(itemData.ItemConfigId, itemCount))
			{
				return false;
			}
			if (LastAddedCount != itemCount)
			{
				LastError = $"背包空间不足，仅复制 {LastAddedCount}/{itemCount} 个";
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "Duplicate 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool RemoveBackpackItem(long instanceId)
	{
		WebUiBridge.SetOp("GameApi.RemoveBackpackItem");
		try
		{
			ItemManager items = Items;
			if (items == null)
			{
				LastError = "物品管理器不可用";
				return false;
			}
			if (items.GetItemData(instanceId) == null)
			{
				LastError = "未找到该物品";
				return false;
			}
			items.ForceRemoveItem(instanceId);
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "Remove 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	private static bool IsMaxAttr(AttrName name)
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0003: Unknown result type (might be due to invalid IL or missing references)
		//IL_0005: Invalid comparison between Unknown and I4
		if ((int)name - 101 <= 4)
		{
			return true;
		}
		return false;
	}

	private static bool TryGetCoreAttrForMax(AttrName maxName, out AttrName coreName)
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0003: Unknown result type (might be due to invalid IL or missing references)
		//IL_001d: Expected I4, but got Unknown
		switch ((int)maxName - 101)
		{
		case 0:
			coreName = (AttrName)1;
			return true;
		case 1:
			coreName = (AttrName)2;
			return true;
		case 2:
			coreName = (AttrName)3;
			return true;
		case 3:
			coreName = (AttrName)4;
			return true;
		case 4:
			coreName = (AttrName)5;
			return true;
		default:
			coreName = (AttrName)0;
			return false;
		}
	}

	private static bool TryGetMaxAttrForCore(AttrName coreName, out AttrName maxName)
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_001c: Expected I4, but got Unknown
		switch ((int)coreName - 1)
		{
		case 0:
			maxName = (AttrName)101;
			return true;
		case 1:
			maxName = (AttrName)102;
			return true;
		case 2:
			maxName = (AttrName)103;
			return true;
		case 3:
			maxName = (AttrName)104;
			return true;
		case 4:
			maxName = (AttrName)105;
			return true;
		default:
			maxName = (AttrName)0;
			return false;
		}
	}

	private static bool TryGetAttrEntry(AttributeComponent component, AttrName name, out Attr attr)
	{
		//IL_0014: Unknown result type (might be due to invalid IL or missing references)
		//IL_0022: Unknown result type (might be due to invalid IL or missing references)
		attr = null;
		try
		{
			Dictionary<AttrName, Attr> val = ((component != null) ? component.AttrDict : null);
			if (val == null || !val.ContainsKey(name))
			{
				return false;
			}
			attr = val[name];
			return attr != null;
		}
		catch
		{
			return false;
		}
	}

	private static int AttrScale()
	{
		float attr_ScalingRatio = GameKey.Attr_ScalingRatio;
		if (!float.IsFinite(attr_ScalingRatio) || attr_ScalingRatio <= 0f)
		{
			return 1000;
		}
		return Mathf.RoundToInt(attr_ScalingRatio);
	}

	public static int GetAttr(AttrName name)
	{
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			if (playerAttributeComponent == null)
			{
				return -1;
			}
			return Mathf.RoundToInt(playerAttributeComponent.GetTotalValue_Float(name));
		}
		catch
		{
			return -1;
		}
	}

	public static int GetMaxAttr(AttrName name)
	{
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			if (playerAttributeComponent == null)
			{
				return -1;
			}
			return playerAttributeComponent.GetTotalValue_Int(name);
		}
		catch
		{
			return -1;
		}
	}

	public static long GetAttributeSourceId()
	{
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			return (playerAttributeComponent != null) ? ((Il2CppObjectBase)playerAttributeComponent).Pointer.ToInt64() : 0;
		}
		catch
		{
			return 0L;
		}
	}

	public static bool SetAttr(AttrName name, int value)
	{
		//IL_0024: Unknown result type (might be due to invalid IL or missing references)
		//IL_0047: Unknown result type (might be due to invalid IL or missing references)
		//IL_002d: Unknown result type (might be due to invalid IL or missing references)
		//IL_008f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0051: Unknown result type (might be due to invalid IL or missing references)
		WebUiBridge.SetOp("GameApi.SetAttr");
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			if (playerAttributeComponent == null)
			{
				LastError = "属性组件不可用";
				return false;
			}
			if (IsMaxAttr(name))
			{
				if (!SetMaxAttrDirect(playerAttributeComponent, name, value))
				{
					LastError = "未找到对应的属性上限";
					return false;
				}
			}
			else
			{
				if (TryGetMaxAttrForCore(name, out var maxName))
				{
					int maxAttr = GetMaxAttr(maxName);
					if (maxAttr >= 0 && value > maxAttr)
					{
						LastError = $"当前值不能超过属性上限 {maxAttr}";
						return false;
					}
				}
				if (!SetCurrentAttrDirect(playerAttributeComponent, name, value))
				{
					LastError = "未找到对应的角色属性";
					return false;
				}
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "SetAttr 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	private static bool SetCurrentAttrDirect(AttributeComponent component, AttrName name, int value)
	{
		//IL_0001: Unknown result type (might be due to invalid IL or missing references)
		//IL_0019: Unknown result type (might be due to invalid IL or missing references)
		//IL_0023: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a3: Unknown result type (might be due to invalid IL or missing references)
		if (!TryGetAttrEntry(component, name, out var attr))
		{
			return false;
		}
		int num = AttrScale();
		long num2 = (long)value * (long)num;
		if (TryGetMaxAttrForCore(name, out var maxName))
		{
			int maxAttr = GetMaxAttr(maxName);
			if (maxAttr >= 0)
			{
				long num3 = (long)maxAttr * (long)num;
				num2 = Math.Min(num2, num3);
				int num4 = SaturatingInt(num3);
				if (attr._Max_k__BackingField < num4)
				{
					attr._Max_k__BackingField = num4;
				}
			}
		}
		else if (attr._Max_k__BackingField < num2)
		{
			attr._Max_k__BackingField = SaturatingInt(num2);
		}
		if (attr._Min_k__BackingField > num2)
		{
			attr._Min_k__BackingField = SaturatingInt(num2);
		}
		int strengtheningValue_k__BackingField = attr._StrengtheningValue_k__BackingField;
		attr._BaseValue_k__BackingField = SaturatingInt(num2 - strengtheningValue_k__BackingField);
		component.SyncAttr2UI(name);
		return true;
	}

	private static int SaturatingInt(long value)
	{
		if (value <= int.MaxValue)
		{
			if (value >= int.MinValue)
			{
				return (int)value;
			}
			return int.MinValue;
		}
		return int.MaxValue;
	}

	internal static bool EnsureMaxAttrCanGrow(AttrName maxName)
	{
		//IL_0009: Unknown result type (might be due to invalid IL or missing references)
		//IL_0012: Unknown result type (might be due to invalid IL or missing references)
		//IL_0036: Unknown result type (might be due to invalid IL or missing references)
		//IL_0041: Unknown result type (might be due to invalid IL or missing references)
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			if (playerAttributeComponent == null || !IsMaxAttr(maxName) || !TryGetAttrEntry(playerAttributeComponent, maxName, out var attr))
			{
				LastError = "未找到对应的属性上限";
				return false;
			}
			attr._Max_k__BackingField = int.MaxValue;
			if (TryGetCoreAttrForMax(maxName, out var coreName) && TryGetAttrEntry(playerAttributeComponent, coreName, out var attr2))
			{
				attr2._Max_k__BackingField = int.MaxValue;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "解除属性成长边界失败: " + ex.Message;
			return false;
		}
	}

	private static bool SetMaxAttrDirect(AttributeComponent component, AttrName maxName, int value)
	{
		//IL_0001: Unknown result type (might be due to invalid IL or missing references)
		//IL_0014: Unknown result type (might be due to invalid IL or missing references)
		//IL_0017: Unknown result type (might be due to invalid IL or missing references)
		//IL_0022: Unknown result type (might be due to invalid IL or missing references)
		//IL_002d: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ac: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a5: Unknown result type (might be due to invalid IL or missing references)
		//IL_009a: Unknown result type (might be due to invalid IL or missing references)
		if (!TryGetAttrEntry(component, maxName, out var attr))
		{
			return false;
		}
		float num = -1f;
		AttrName coreName = (AttrName)0;
		Attr attr2 = null;
		if (TryGetCoreAttrForMax(maxName, out coreName) && TryGetAttrEntry(component, coreName, out attr2))
		{
			num = component.GetTotalValue_Float(coreName);
		}
		attr._BaseValue_k__BackingField = SaturatingInt((long)value - (long)attr._StrengtheningValue_k__BackingField);
		attr._Max_k__BackingField = int.MaxValue;
		if (attr._Min_k__BackingField > value)
		{
			attr._Min_k__BackingField = value;
		}
		if (attr2 != null)
		{
			int num2 = SaturatingInt((long)value * (long)AttrScale());
			attr2._Max_k__BackingField = int.MaxValue;
			if (attr2._Min_k__BackingField > num2)
			{
				attr2._Min_k__BackingField = num2;
			}
			if (num > (float)value)
			{
				SetCurrentAttrDirect(component, coreName, value);
			}
			else
			{
				component.SyncAttr2UI(coreName);
			}
		}
		component.SyncAttr2UI(maxName);
		return true;
	}

	public static bool TryGetMoveSpeed(out float current, out float original, out float multiplier)
	{
		current = -1f;
		original = -1f;
		multiplier = 1f;
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			if (!EnsureMoveSpeedBaseline(playerAttributeComponent))
			{
				return false;
			}
			current = playerAttributeComponent.GetTotalValue_Float((AttrName)401, (AttrName)10001);
			if (!IsValidSpeed(current))
			{
				current = playerAttributeComponent.GetTotalValue_Float((AttrName)401);
			}
			original = _originalMoveSpeedBase;
			multiplier = _moveSpeedMultiplier;
			return IsValidSpeed(current);
		}
		catch (Exception ex)
		{
			LogErr("TryGetMoveSpeed: " + ex.Message);
			return false;
		}
	}

	public static bool SetMoveSpeedMultiplier(float multiplier)
	{
		WebUiBridge.SetOp("GameApi.SetMoveSpeedMultiplier");
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			if (!EnsureMoveSpeedBaseline(playerAttributeComponent))
			{
				LastError = "移动速度属性不可用，请先进入游戏存档";
				return false;
			}
			if (!float.IsFinite(multiplier) || multiplier < 0.5f || multiplier > 5f)
			{
				LastError = "移动速度倍率必须在 0.5 到 5 之间";
				return false;
			}
			playerAttributeComponent.SetBaseValue_Float((AttrName)401, _originalMoveSpeedBase * multiplier, true);
			playerAttributeComponent.SyncAttr2UI((AttrName)401);
			_moveSpeedMultiplier = multiplier;
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "设置移动速度失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool ResetMoveSpeed()
	{
		WebUiBridge.SetOp("GameApi.ResetMoveSpeed");
		try
		{
			AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
			if (!EnsureMoveSpeedBaseline(playerAttributeComponent))
			{
				LastError = "移动速度属性不可用，请先进入游戏存档";
				return false;
			}
			playerAttributeComponent.SetBaseValue_Float((AttrName)401, _originalMoveSpeedBase, true);
			playerAttributeComponent.SyncAttr2UI((AttrName)401);
			_moveSpeedMultiplier = 1f;
			float baseValue_Float = playerAttributeComponent.GetBaseValue_Float((AttrName)401);
			if (!IsValidSpeed(baseValue_Float) || Math.Abs(baseValue_Float - _originalMoveSpeedBase) > 0.001f)
			{
				LastError = $"移动速度恢复失败，当前基础速度为 {baseValue_Float:0.###}";
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "恢复移动速度失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	private static bool EnsureMoveSpeedBaseline(AttributeComponent component)
	{
		if (component == null)
		{
			return false;
		}
		IntPtr pointer = ((Il2CppObjectBase)component).Pointer;
		if (pointer == IntPtr.Zero)
		{
			return false;
		}
		if (_moveSpeedOwnerPointer == pointer && IsValidSpeed(_originalMoveSpeedBase))
		{
			return true;
		}
		float num = GetConfiguredMoveSpeed();
		if (!IsValidSpeed(num))
		{
			num = component.GetBaseValue_Float((AttrName)401);
		}
		if (!IsValidSpeed(num))
		{
			return false;
		}
		_moveSpeedOwnerPointer = pointer;
		_originalMoveSpeedBase = num;
		_moveSpeedMultiplier = 1f;
		return true;
	}

	private static float GetConfiguredMoveSpeed()
	{
		try
		{
			LeadingRole player = Player;
			ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
			if (player == null || instance == null || ((BaseAgent)player).AgentConfigId <= 0)
			{
				return -1f;
			}
			Config_Player val = instance.Get_Config_Player(((BaseAgent)player).AgentConfigId);
			if (val == null || val.LinkAttr <= 0)
			{
				return -1f;
			}
			Config_PlayAttribute obj = instance.Get_Config_PlayAttribute(val.LinkAttr);
			return (obj != null) ? obj.MoveSpeed : (-1f);
		}
		catch (Exception ex)
		{
			LogErr("GetConfiguredMoveSpeed: " + ex.Message);
			return -1f;
		}
	}

	private static bool IsValidSpeed(float value)
	{
		if (float.IsFinite(value))
		{
			return value > 0f;
		}
		return false;
	}

	public static bool SetHomeDurability(int slotTypeId, int value, out int updated)
	{
		updated = 0;
		WebUiBridge.SetOp("GameApi.SetHomeDurability");
		try
		{
			AgentManager agents = Agents;
			if (agents == null)
			{
				LastError = "设施管理器不可用，请先进入游戏存档";
				return false;
			}
			List<Furniture> homeFurnituresBySlotType = agents.GetHomeFurnituresBySlotType(slotTypeId);
			if (homeFurnituresBySlotType == null)
			{
				LastError = "没有找到对应的住宅设施";
				return false;
			}
			Il2CppSystem.Collections.Generic.List<Furniture>.Enumerator enumerator = homeFurnituresBySlotType.GetEnumerator();
			while (enumerator.MoveNext())
			{
				Furniture current = enumerator.Current;
				FurnitureDurabilityComponent val = ((current != null) ? AgentTools.GetAgentComponent<FurnitureDurabilityComponent>((BaseAgent)(object)current) : null);
				if (val != null)
				{
					val.SetMaxDurability(value);
					val.SetCurrentDurability(value);
					updated++;
				}
			}
			LastError = ((updated > 0) ? "" : "没有找到可修改的门窗，请确认住宅设施已经建造");
			return updated > 0;
		}
		catch (Exception ex)
		{
			LastError = "设置门窗耐久失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static void GetHomeDurabilitySummary(int slotTypeId, out int count, out int minimumCurrent, out int maximum)
	{
		count = 0;
		minimumCurrent = -1;
		maximum = -1;
		try
		{
			AgentManager agents = Agents;
			List<Furniture> val = ((agents != null) ? agents.GetHomeFurnituresBySlotType(slotTypeId) : null);
			if (val == null)
			{
				return;
			}
			Il2CppSystem.Collections.Generic.List<Furniture>.Enumerator enumerator = val.GetEnumerator();
			while (enumerator.MoveNext())
			{
				Furniture current = enumerator.Current;
				FurnitureDurabilityComponent val2 = ((current != null) ? AgentTools.GetAgentComponent<FurnitureDurabilityComponent>((BaseAgent)(object)current) : null);
				if (val2 != null)
				{
					int currentDurability = val2.GetCurrentDurability();
					int maxDurability = val2.GetMaxDurability();
					minimumCurrent = ((minimumCurrent < 0) ? currentDurability : Math.Min(minimumCurrent, currentDurability));
					maximum = Math.Max(maximum, maxDurability);
					count++;
				}
			}
		}
		catch (Exception ex)
		{
			LogErr("GetHomeDurabilitySummary: " + ex.Message);
		}
	}

	public static bool SetInfiniteFoodShelfLife(bool enabled, out int affected)
	{
		affected = 0;
		WebUiBridge.SetOp("GameApi.SetInfiniteFoodShelfLife");
		try
		{
			ItemManager items = Items;
			if (items == null)
			{
				LastError = "物品管理器不可用，请先进入游戏存档";
				return false;
			}
			Dictionary<long, ItemData> cache = items.Cache;
			if (cache != null)
			{
				Il2CppSystem.Collections.Generic.Dictionary<long, ItemData>.Enumerator enumerator = cache.GetEnumerator();
				while (enumerator.MoveNext())
				{
					ItemData value = enumerator.Current.Value;
					object obj;
					if (value != null)
					{
						ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
						obj = ((instance != null) ? instance.Get_Config_Item(value.ItemConfigId) : null);
					}
					else
					{
						obj = null;
					}
					Config_Item val = (Config_Item)obj;
					if (val != null && val.Life > 0)
					{
						affected++;
					}
				}
			}
			InfiniteFoodShelfLife = enabled;
			if (!enabled)
			{
				items.RefreshAllItemTimeScale();
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			InfiniteFoodShelfLife = false;
			LastError = "切换无限食物保质期失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static void ApplyAttrLocks(bool hp, bool stamina, bool satiety, bool morale)
	{
		if (!hp && !stamina && !satiety && !morale)
		{
			return;
		}
		WebUiBridge.SetOp("GameApi.ApplyAttrLocks");
		AttributeComponent playerAttributeComponent = PlayerAttributeComponent;
		if (playerAttributeComponent == null)
		{
			WebUiBridge.SetOp(null);
			return;
		}
		try
		{
			if (hp)
			{
				TopUp(playerAttributeComponent, (AttrName)5, (AttrName)105);
			}
			if (stamina)
			{
				TopUp(playerAttributeComponent, (AttrName)3, (AttrName)103);
			}
			if (satiety)
			{
				TopUp(playerAttributeComponent, (AttrName)1, (AttrName)101);
			}
			if (morale)
			{
				TopUp(playerAttributeComponent, (AttrName)2, (AttrName)102);
			}
		}
		catch (Exception ex)
		{
			LogErr("ApplyAttrLocks: " + ex.Message);
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	private static void TopUp(AttributeComponent attr, AttrName curName, AttrName maxName)
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_002a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0040: Unknown result type (might be due to invalid IL or missing references)
		try
		{
			int lockValue = WebUiBridge.GetLockValue(curName);
			int maxAttr = GetMaxAttr(maxName);
			int num = ((lockValue >= 0) ? lockValue : maxAttr);
			if (num >= 0)
			{
				if (maxAttr >= 0)
				{
					num = Math.Min(num, maxAttr);
				}
				if (Math.Abs(attr.GetTotalValue_Float(curName) - (float)num) > 0.001f)
				{
					SetCurrentAttrDirect(attr, curName, num);
				}
			}
		}
		catch
		{
		}
	}

	public static int GameDay()
	{
		try
		{
			GameTimeManager time = Time;
			return (time != null) ? time.GetDay() : (-1);
		}
		catch
		{
			return -1;
		}
	}

	public static int GameHour()
	{
		try
		{
			GameTimeManager time = Time;
			return (time != null) ? time.GetHour() : (-1);
		}
		catch
		{
			return -1;
		}
	}

	public static int GameTotalSeconds()
	{
		try
		{
			GameTimeManager time = Time;
			return (time != null) ? time.GetTotalSeconds() : (-1);
		}
		catch
		{
			return -1;
		}
	}

	public static bool IsClockFrozen()
	{
		try
		{
			return FrozenOverride || (Time != null && Time.IsClockFrozen);
		}
		catch
		{
			return FrozenOverride;
		}
	}

	public static float RemainCountdownHour()
	{
		try
		{
			GameTimeManager time = Time;
			return (time != null) ? time.GetRemainHourFloat() : (-1f);
		}
		catch
		{
			return -1f;
		}
	}

	public static string TimerTypeName()
	{
		try
		{
			GameTimeManager time = Time;
			ITimer val = ((time != null) ? time.Timer : null);
			if (val == null || ((Il2CppObjectBase)val).Pointer == IntPtr.Zero)
			{
				return "";
			}
			IntPtr intPtr = IL2CPP.il2cpp_object_get_class(((Il2CppObjectBase)val).Pointer);
			if (intPtr != IntPtr.Zero)
			{
				string text = IL2CPP.il2cpp_class_get_name_(intPtr);
				if (!string.IsNullOrEmpty(text))
				{
					return text;
				}
			}
			return ((object)val).GetType().Name;
		}
		catch
		{
			return "";
		}
	}

	public static bool ExtendCountdown(int hours)
	{
		//IL_007c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0082: Expected O, but got Unknown
		WebUiBridge.SetOp("GameApi.ExtendCountdown");
		checked
		{
			try
			{
				GameTimeManager time = Time;
				if (time == null)
				{
					LastError = "时间管理器不可用（可能不在游戏中）";
					return false;
				}
				if (TimerTypeName() != "CountDownTimer")
				{
					LastError = "延长倒计时仅在准备阶段生效";
					return false;
				}
				ITimer timer = time.Timer;
				if (timer == null || ((Il2CppObjectBase)timer).Pointer == IntPtr.Zero)
				{
					LastError = "准备阶段倒计时尚未加载";
					return false;
				}
				CountDownTimer val = new CountDownTimer(((Il2CppObjectBase)timer).Pointer);
				int num = hours * 3600;
				float remainHourFloat = val.GetRemainHourFloat();
				float remainTime = val.RemainTime;
				float totalTime = val.TotalTime;
				int extraCountDownTimeConfig = val.ExtraCountDownTimeConfig;
				time.AddExtraCountDownTime(num);
				float remainHourFloat2 = val.GetRemainHourFloat();
				if (!float.IsFinite(remainHourFloat2) || remainHourFloat2 <= remainHourFloat + 0.0001f)
				{
					float gameTime_Float = Toolset.GetGameTime_Float(num);
					if (!float.IsFinite(gameTime_Float) || gameTime_Float <= 0f)
					{
						LastError = "无法换算准备阶段时间增量";
						return false;
					}
					val.RemainTime = remainTime + gameTime_Float;
					val.TotalTime = totalTime + gameTime_Float;
					val.IsCompleted = false;
					if (val.ExtraCountDownTimeConfig <= extraCountDownTimeConfig)
					{
						val.ExtraCountDownTimeConfig = extraCountDownTimeConfig + num;
					}
				}
				time.AddExtraCountDownTime(0);
				time.NotifyCountDownTimerScaleSync();
				remainHourFloat2 = val.GetRemainHourFloat();
				if (!float.IsFinite(remainHourFloat2) || remainHourFloat2 <= remainHourFloat)
				{
					LastError = "倒计时未发生变化，请确认当前仍在准备阶段";
					return false;
				}
				LogInfo($"[时间] 延长 {hours}h：{remainHourFloat:F2}h -> {remainHourFloat2:F2}h");
				LastError = "";
				return true;
			}
			catch (Exception ex)
			{
				LastError = "ExtendCountdown 异常: " + ex.Message;
				LogErr(LastError);
				return false;
			}
			finally
			{
				WebUiBridge.SetOp(null);
			}
		}
	}

	public static bool SetTimeFrozen(bool on)
	{
		WebUiBridge.SetOp("GameApi.SetTimeFrozen");
		try
		{
			GameTimeManager time = Time;
			if (time == null)
			{
				LastError = "时间管理器不可用";
				return false;
			}
			time.IsClockFrozen = on;
			FrozenOverride = on;
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "SetTimeFrozen 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static void ApplyFrozenOverride()
	{
		if (!FrozenOverride)
		{
			return;
		}
		try
		{
			GameTimeManager time = Time;
			if (time != null && !time.IsClockFrozen)
			{
				time.IsClockFrozen = true;
			}
		}
		catch
		{
		}
	}

	public static int RemoveAllNegativeBuffs()
	{
		WebUiBridge.SetOp("GameApi.RemoveAllNegativeBuffs");
		int num = 0;
		try
		{
			BuffComponent playerBuff = PlayerBuff;
			if (playerBuff == null)
			{
				LastError = "Buff 组件不可用";
				return 0;
			}
			List<int> buffConfigIds = playerBuff.GetBuffConfigIds();
			if (buffConfigIds == null)
			{
				LastError = "无法读取 Buff 列表";
				return 0;
			}
			System.Collections.Generic.HashSet<int> hashSet = new System.Collections.Generic.HashSet<int>();
			try
			{
				ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
				Dictionary<int, Config_Buff> val = ((instance != null) ? instance._Config_Buff_Dict : null);
				if (val == null)
				{
					LastError = "Buff 配置尚未加载";
					return 0;
				}
				var enumerator = val.GetEnumerator();
				while (enumerator.MoveNext())
				{
					KeyValuePair<int, Config_Buff> current = enumerator.Current;
					try
					{
						if (current.Value != null && !current.Value.IsGood)
						{
							hashSet.Add(current.Key);
						}
					}
					catch
					{
					}
				}
			}
			catch (Exception ex)
			{
				LastError = "读取 Buff 配置失败: " + ex.Message;
				LogErr(LastError);
				return 0;
			}
			List<int> list = SnapshotIds(buffConfigIds);
			HashSet<int> activeSurvivalPlanBuffIds = GetActiveSurvivalPlanBuffIds();
			int num2 = 0;
			for (int i = 0; i < list.Count; i++)
			{
				int num3 = list[i];
				if (hashSet.Contains(num3) && !activeSurvivalPlanBuffIds.Contains(num3))
				{
					try
					{
						playerBuff.RemoveBuff(num3);
						num++;
					}
					catch
					{
						num2++;
					}
				}
			}
			try
			{
				playerBuff.RequestLeadingRoleBuffRefresh();
			}
			catch
			{
			}
			LastError = ((num2 == 0) ? "" : $"已移除 {num} 个负面效果，另有 {num2} 个移除失败");
			return num;
		}
		catch (Exception ex2)
		{
			LastError = "RemoveAllNegativeBuffs 异常: " + ex2.Message;
			LogErr(LastError);
			return num;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool ClearAllBuffs()
	{
		WebUiBridge.SetOp("GameApi.ClearAllBuffs");
		try
		{
			BuffComponent playerBuff = PlayerBuff;
			if (playerBuff == null)
			{
				LastError = "Buff 组件不可用";
				return false;
			}
			List<int> buffConfigIds = playerBuff.GetBuffConfigIds();
			if (buffConfigIds == null)
			{
				LastError = "无法读取 Buff 列表";
				return false;
			}
			List<int> list = SnapshotIds(buffConfigIds);
			HashSet<int> activeSurvivalPlanBuffIds = GetActiveSurvivalPlanBuffIds();
			int num = 0;
			for (int i = 0; i < list.Count; i++)
			{
				int num2 = list[i];
				if (!activeSurvivalPlanBuffIds.Contains(num2))
				{
					try
					{
						playerBuff.RemoveBuff(num2);
					}
					catch
					{
						num++;
					}
				}
			}
			try
			{
				playerBuff.RequestLeadingRoleBuffRefresh();
			}
			catch
			{
			}
			LastError = ((num == 0) ? "" : $"有 {num} 个 Buff 移除失败");
			return num == 0;
		}
		catch (Exception ex)
		{
			LastError = "ClearAllBuffs 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static RelationshipView GetRelationship()
	{
		RelationshipView relationshipView = new RelationshipView();
		try
		{
			NeighborRescueManager neighbor = Neighbor;
			if (neighbor == null)
			{
				LastError = "邻居关系管理器不可用（可能不在游戏中）";
				return relationshipView;
			}
			relationshipView.Affinity = neighbor.GmGetAffinity();
			relationshipView.Tier = neighbor.GetAffinityDisplayTier();
			relationshipView.MaxAffinity = GetNeighborAffinityMax();
			relationshipView.Locked = !neighbor.GmIsUnlocked();
			try
			{
				relationshipView.TierName = NeighborRules.GetTierName(relationshipView.Tier);
			}
			catch
			{
			}
			relationshipView.Name = (relationshipView.Locked ? "可攻略邻居（未解锁）" : "可攻略邻居（已解锁）");
			LastError = "";
		}
		catch (Exception ex)
		{
			LastError = "GetRelationship 异常: " + ex.Message;
			LogErr(LastError);
		}
		return relationshipView;
	}

	public static bool SetRelationship(int value)
	{
		WebUiBridge.SetOp("GameApi.SetRelationship");
		try
		{
			NeighborRescueManager neighbor = Neighbor;
			if (neighbor == null)
			{
				LastError = "邻居关系管理器不可用";
				return false;
			}
			if (!neighbor.GmIsUnlocked())
			{
				LastError = "可攻略邻居尚未解锁，不能修改好感度";
				return false;
			}
			int neighborAffinityMax = GetNeighborAffinityMax();
			if (value > neighborAffinityMax)
			{
				LastError = $"好感度范围为 0 到 {neighborAffinityMax}";
				return false;
			}
			int num = neighbor.GmGetAffinity();
			neighbor.AddAffinity(value - num, PlayerId);
			int num2 = neighbor.GmGetAffinity();
			if (num2 != value)
			{
				LastError = $"好感度修改未生效，当前仍为 {num2}";
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "SetRelationship 异常: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool UnlockAllCodex(out string details)
	{
		details = "";
		WebUiBridge.SetOp("GameApi.UnlockAllCodex");
		try
		{
			CodexManager codex = Codex;
			if (codex == null)
			{
				LastError = "图鉴管理器不可用，请先进入游戏存档";
				return false;
			}
			codex.GmUnlockAll();
			details = "全图鉴已解锁并写入当前存档";
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "解锁全图鉴失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool UnlockAllAchievements(out string details)
	{
		details = "";
		WebUiBridge.SetOp("GameApi.UnlockAllAchievements");
		try
		{
			AchievementManager achievements = Achievements;
			if (achievements == null)
			{
				LastError = "成就管理器不可用，请先进入游戏存档";
				return false;
			}
			details = achievements.GmUnlockAll();
			if (string.IsNullOrWhiteSpace(details))
			{
				details = "全成就已解锁";
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "解锁全成就失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	private static List<int> SnapshotIds(List<int> source)
	{
		List<int> list = new List<int>();
		if (source == null)
		{
			return list;
		}
		var enumerator = source.GetEnumerator();
		while (enumerator.MoveNext())
		{
			list.Add(enumerator.Current);
		}
		return list;
	}

	public static bool SetNoExploreExposure(bool enabled)
	{
		WebUiBridge.SetOp("GameApi.SetNoExploreExposure");
		try
		{
			WebGm.LockExploreExposure = enabled;
			NoExploreExposure = enabled;
			_noExposureErrorLogged = false;
			if (enabled)
			{
				ExploreManager explore = Explore;
				if (explore != null)
				{
					explore._CurExposure_k__BackingField = 0f;
					explore.NotifyUI();
				}
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "设置防暴露失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static void ApplyNoExploreExposure()
	{
		if (!NoExploreExposure)
		{
			return;
		}
		try
		{
			WebGm.LockExploreExposure = true;
			ExploreManager explore = Explore;
			if (explore != null && explore.CurExposure > 0f)
			{
				explore._CurExposure_k__BackingField = 0f;
				explore.NotifyUI();
			}
		}
		catch (Exception ex)
		{
			if (!_noExposureErrorLogged)
			{
				_noExposureErrorLogged = true;
				LogErr("ApplyNoExploreExposure: " + ex.Message);
			}
		}
	}

	public static System.Collections.Generic.List<BuffView> GetBuffsLegacy()
	{
		System.Collections.Generic.List<BuffView> list = new System.Collections.Generic.List<BuffView>();
		try
		{
			BuffComponent playerBuff = PlayerBuff;
			if (playerBuff == null)
			{
				LastError = "Buff 组件不可用";
				return list;
			}
			Dictionary<long, BuffArgs> editorBuffMap = playerBuff.GetEditorBuffMap();
			if (editorBuffMap == null)
			{
				LastError = "无法读取 Buff 列表";
				return list;
			}
			var enumerator = editorBuffMap.GetEnumerator();
			while (enumerator.MoveNext())
			{
				KeyValuePair<long, BuffArgs> current = enumerator.Current;
				BuffArgs value = current.Value;
				if (value != null && value.config != null)
				{
					string text = value.config.Name_Local ?? value.config.Name;
					list.Add(new BuffView
					{
						InstanceId = current.Key,
						ConfigId = value.config.ID,
						Name = (string.IsNullOrEmpty(text) ? ("Buff #" + value.config.ID) : text),
						IsGood = value.config.IsGood,
						Layers = value.BuffCount,
						TimeEndTime = value.TimeEndTime
					});
				}
			}
			// // list.Sort((BuffView left, BuffView right) => (left.ConfigId == right.ConfigId) ? left.InstanceId.CompareTo(right.InstanceId) : left.ConfigId.CompareTo(right.ConfigId)); // il2cpp IComparer 不兼容 // il2cpp IComparer 不兼容
			LastError = "";
		}
		catch (Exception ex)
		{
			LastError = "读取 Buff 失败: " + ex.Message;
			LogErr(LastError);
		}
		return list;
	}

	public static System.Collections.Generic.List<BuffView> GetBuffs()
	{
		System.Collections.Generic.List<BuffView> buffsLegacy = GetBuffsLegacy();
		try
		{
			BuffComponent playerBuff = PlayerBuff;
			if (playerBuff == null)
			{
				return buffsLegacy;
			}
			System.Collections.Generic.HashSet<long> hashSet = new System.Collections.Generic.HashSet<long>();
			for (int i = 0; i < buffsLegacy.Count; i++)
			{
				if (buffsLegacy[i].InstanceId > 0)
				{
					hashSet.Add(buffsLegacy[i].InstanceId);
				}
			}
			System.Collections.Generic.HashSet<int> hashSet2 = new System.Collections.Generic.HashSet<int>();
			System.Collections.Generic.HashSet<long> hashSet3 = new System.Collections.Generic.HashSet<long>();
			List<BuffArgs> effectiveBuffList = playerBuff.GetEffectiveBuffList();
			if (effectiveBuffList != null)
			{
				var enumerator = effectiveBuffList.GetEnumerator();
				while (enumerator.MoveNext())
				{
					BuffArgs current = enumerator.Current;
					if (current == null || current.config == null)
					{
						continue;
					}
					long num = current.EffectBtArgsID;
					if (num <= 0)
					{
						num = current.buffShowInstanceId;
					}
					bool flag = num > 0 && hashSet.Contains(num);
					if (flag)
					{
						hashSet.Remove(num);
						continue;
					}
					if (num > 0)
					{
						if (!hashSet3.Add(num))
						{
							continue;
						}
					}
					else if (!hashSet2.Add(current.config.ID))
					{
						continue;
					}
					string text = current.config.Name_Local ?? current.config.Name;
					buffsLegacy.Add(new BuffView
					{
						InstanceId = (flag ? num : 0),
						ConfigId = current.config.ID,
						Name = (string.IsNullOrEmpty(text) ? ("Buff #" + current.config.ID) : text),
						IsGood = current.config.IsGood,
						Layers = current.BuffCount,
						TimeEndTime = current.TimeEndTime,
						RemoveByConfig = true,
						Source = "当前生效/生存规划"
					});
				}
			}
			// // buffsLegacy.Sort((BuffView left, BuffView right) => (left.ConfigId == right.ConfigId) ? left.InstanceId.CompareTo(right.InstanceId) : left.ConfigId.CompareTo(right.ConfigId)); // il2cpp IComparer 不兼容 // il2cpp IComparer 不兼容
			LastError = "";
		}
		catch (Exception ex)
		{
			LogErr("GetEffectiveBuffList: " + ex.Message);
		}
		return buffsLegacy;
	}

	private static Config_Talent FindTalentConfig(int talentId)
	{
		if (talentId <= 0)
		{
			return null;
		}
		try
		{
			ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
			Config_Talent val = ((instance != null) ? instance.Get_Config_Talent(talentId) : null);
			if (val != null)
			{
				return val;
			}
		}
		catch
		{
		}
		try
		{
			ConfigManager instance2 = BaseSingleton<ConfigManager>.Instance;
			Config_Talent val2 = ((instance2 != null) ? instance2.GetTalentConfig(talentId, 1) : null);
			if (val2 != null)
			{
				return val2;
			}
		}
		catch
		{
		}
		Dictionary<int, Config_Talent> val4 = default(Dictionary<int, Config_Talent>);
		for (int i = 1; i <= 2; i++)
		{
			try
			{
				ConfigManager instance3 = BaseSingleton<ConfigManager>.Instance;
				Dictionary<int, Dictionary<int, Config_Talent>> val3 = ((instance3 != null) ? instance3.GetTalentsByShowType(i) : null);
				if (val3 == null || !val3.TryGetValue(talentId, out val4) || val4 == null)
				{
					continue;
				}
				var enumerator = val4.GetEnumerator();
				Config_Talent val5 = null;
				while (enumerator.MoveNext())
				{
					Config_Talent value = enumerator.Current.Value;
					if (value != null && (val5 == null || value.Lv < val5.Lv))
					{
						val5 = value;
					}
				}
				if (val5 == null)
				{
					continue;
				}
				return val5;
			}
			catch
			{
			}
		}
		return null;
	}

	public static System.Collections.Generic.List<SurvivalPlanView> GetSurvivalPlans()
	{
		System.Collections.Generic.List<SurvivalPlanView> list = new System.Collections.Generic.List<SurvivalPlanView>();
		try
		{
			SurvivalPlanningComponent playerSurvivalPlanning = PlayerSurvivalPlanning;
			List<int> val = ((playerSurvivalPlanning != null) ? playerSurvivalPlanning.SaveCache : null);
			if (val == null)
			{
				LastError = "生存规划尚未加载";
				return list;
			}
			var enumerator = val.GetEnumerator();
			while (enumerator.MoveNext())
			{
				int current = enumerator.Current;
				Config_Talent val2 = FindTalentConfig(current);
				string text = ((val2 != null) ? val2.Name_Local : null) ?? ((val2 != null) ? val2.Name : null);
				list.Add(new SurvivalPlanView
				{
					TalentId = current,
					Name = (string.IsNullOrEmpty(text) ? ("生存规划 #" + current) : text),
					Description = (((val2 != null) ? val2.Dec : null) ?? ""),
					Level = ((val2 != null) ? val2.Lv : 0),
					Active = playerSurvivalPlanning.HasActivated(current)
				});
			}
			// list.Sort((SurvivalPlanView left, SurvivalPlanView right) => left.TalentId.CompareTo(right.TalentId)); // il2cpp IComparer 不兼容
			LastError = "";
		}
		catch (Exception ex)
		{
			LastError = "读取生存规划失败: " + ex.Message;
			LogErr(LastError);
		}
		return list;
	}

	private static void AddConfiguredSurvivalPlanIds(System.Collections.Generic.HashSet<int> target, List<int> ids, ConfigManager manager)
	{
		if (target == null || ids == null || manager == null)
		{
			return;
		}
		var enumerator = ids.GetEnumerator();
		while (enumerator.MoveNext())
		{
			int current = enumerator.Current;
			if (current <= 0 || target.Contains(current))
			{
				continue;
			}
			try
			{
				if (manager.Get_Config_Talent(current) != null)
				{
					target.Add(current);
				}
			}
			catch
			{
			}
		}
	}

	private static System.Collections.Generic.HashSet<int> GetConfiguredSurvivalPlanIds()
	{
		System.Collections.Generic.HashSet<int> hashSet = new System.Collections.Generic.HashSet<int>();
		ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
		Dictionary<int, Config_DailyRandom> val = ((instance != null) ? instance._Config_DailyRandom_Dict : null);
		if (instance == null || val == null)
		{
			return hashSet;
		}
		var enumerator = val.GetEnumerator();
		while (enumerator.MoveNext())
		{
			Config_DailyRandom value = enumerator.Current.Value;
			if (value == null)
			{
				continue;
			}
			AddConfiguredSurvivalPlanIds(hashSet, value.SpecifiedPlanID, instance);
			List<int> randomPlanID = value.RandomPlanID;
			if (randomPlanID == null)
			{
				continue;
			}
			var enumerator2 = randomPlanID.GetEnumerator();
			while (enumerator2.MoveNext())
			{
				int current = enumerator2.Current;
				if (current > 0)
				{
					try
					{
						Config_RandomGroup val2 = instance.Get_Config_RandomGroup(current);
						AddConfiguredSurvivalPlanIds(hashSet, (val2 != null) ? val2.IdList : null, instance);
					}
					catch
					{
					}
				}
			}
		}
		return hashSet;
	}

	public static System.Collections.Generic.List<SurvivalPlanView> GetSurvivalPlanCatalog()
	{
		System.Collections.Generic.List<SurvivalPlanView> list = new System.Collections.Generic.List<SurvivalPlanView>();
		try
		{
			SurvivalPlanningComponent playerSurvivalPlanning = PlayerSurvivalPlanning;
			ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
			foreach (int configuredSurvivalPlanId in GetConfiguredSurvivalPlanIds())
			{
				Config_Talent val = null;
				try
				{
					val = ((instance != null) ? instance.Get_Config_Talent(configuredSurvivalPlanId) : null);
				}
				catch
				{
				}
				if (val != null)
				{
					string text = val.Name_Local ?? val.Name;
					list.Add(new SurvivalPlanView
					{
						TalentId = configuredSurvivalPlanId,
						Name = (string.IsNullOrEmpty(text) ? ("生存规划 #" + configuredSurvivalPlanId) : text),
						Description = (val.Dec_Local ?? val.Dec ?? ""),
						Level = val.Lv,
						Active = (playerSurvivalPlanning != null && playerSurvivalPlanning.HasActivated(configuredSurvivalPlanId))
					});
				}
			}
			// list.Sort((SurvivalPlanView left, SurvivalPlanView right) => string.Compare(left.Name, right.Name, StringComparison.Ordinal)); // il2cpp IComparer 不兼容
			LastError = "";
		}
		catch (Exception ex)
		{
			LastError = "读取生存规划目录失败: " + ex.Message;
			LogErr(LastError);
		}
		return list;
	}

	public static bool AddSurvivalPlan(int talentId)
	{
		WebUiBridge.SetOp("GameApi.AddSurvivalPlan");
		try
		{
			SurvivalPlanningComponent playerSurvivalPlanning = PlayerSurvivalPlanning;
			BuffComponent playerBuff = PlayerBuff;
			if (playerSurvivalPlanning == null || playerBuff == null)
			{
				LastError = "生存规划组件尚未加载";
				return false;
			}
			if (talentId <= 0 || !GetConfiguredSurvivalPlanIds().Contains(talentId))
			{
				LastError = "不存在的生存规划 ID";
				return false;
			}
			if (playerSurvivalPlanning.SaveCache == null)
			{
				playerSurvivalPlanning.SaveCache = new List<int>();
			}
			playerSurvivalPlanning.AddBuff(playerBuff, talentId);
			playerSurvivalPlanning.NotifySurvivalPlanningUpdate();
			try
			{
				playerBuff.RequestLeadingRoleBuffRefresh();
			}
			catch
			{
			}
			if (playerSurvivalPlanning.SaveCache == null || !playerSurvivalPlanning.SaveCache.Contains(talentId))
			{
				LastError = "游戏未接受该生存规划，请确认配置和解锁条件";
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "添加生存规划失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool RemoveSurvivalPlan(int talentId)
	{
		WebUiBridge.SetOp("GameApi.RemoveSurvivalPlan");
		try
		{
			SurvivalPlanningComponent playerSurvivalPlanning = PlayerSurvivalPlanning;
			BuffComponent playerBuff = PlayerBuff;
			if (playerSurvivalPlanning == null || playerBuff == null)
			{
				LastError = "生存规划组件尚未加载";
				return false;
			}
			playerSurvivalPlanning.RemoveBuff(playerBuff, talentId);
			playerSurvivalPlanning.NotifySurvivalPlanningUpdate();
			try
			{
				playerBuff.RequestLeadingRoleBuffRefresh();
			}
			catch
			{
			}
			if (playerSurvivalPlanning.SaveCache != null && playerSurvivalPlanning.SaveCache.Contains(talentId))
			{
				LastError = "生存规划仍处于启用状态，移除失败";
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "移除生存规划失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static System.Collections.Generic.List<BuffConfigView> GetBuffConfigs()
	{
		System.Collections.Generic.List<BuffConfigView> list = new System.Collections.Generic.List<BuffConfigView>();
		try
		{
			ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
			Dictionary<int, Config_Buff> val = ((instance != null) ? instance._Config_Buff_Dict : null);
			if (val == null)
			{
				LastError = "Buff 配置尚未加载";
				return list;
			}
			var enumerator = val.GetEnumerator();
			while (enumerator.MoveNext())
			{
				KeyValuePair<int, Config_Buff> current = enumerator.Current;
				Config_Buff value = current.Value;
				if (value != null)
				{
					string text = value.Name_Local ?? value.Name;
					list.Add(new BuffConfigView
					{
						ConfigId = current.Key,
						Name = (string.IsNullOrEmpty(text) ? ("Buff #" + current.Key) : text),
						IsGood = value.IsGood,
						Duration = value.BuffDuring
					});
				}
			}
			// list.Sort((BuffConfigView left, BuffConfigView right) => left.ConfigId.CompareTo(right.ConfigId)); // il2cpp IComparer 不兼容
			LastError = "";
		}
		catch (Exception ex)
		{
			LastError = "读取 Buff 配置失败: " + ex.Message;
			LogErr(LastError);
		}
		return list;
	}

	public static bool AddBuff(int configId)
	{
		WebUiBridge.SetOp("GameApi.AddBuff");
		try
		{
			BuffComponent playerBuff = PlayerBuff;
			if (playerBuff == null)
			{
				LastError = "Buff 组件不可用";
				return false;
			}
			object obj;
			if (configId <= 0)
			{
				obj = null;
			}
			else
			{
				ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
				obj = ((instance != null) ? instance.Get_Config_Buff(configId) : null);
			}
			if (obj == null)
			{
				LastError = "不存在的 Buff 配置 ID";
				return false;
			}
			playerBuff.AddBuff(PlayerId, configId);
			try
			{
				playerBuff.RequestLeadingRoleBuffRefresh();
			}
			catch
			{
			}
			List<int> buffConfigIds = playerBuff.GetBuffConfigIds();
			if (buffConfigIds == null || !buffConfigIds.Contains(configId))
			{
				LastError = "游戏未接受该 Buff，可能不适用于当前角色或场景";
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "添加 Buff 失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool RemoveBuffByConfig(int configId)
	{
		WebUiBridge.SetOp("GameApi.RemoveBuffByConfig");
		try
		{
			BuffComponent playerBuff = PlayerBuff;
			if (playerBuff == null)
			{
				LastError = "Buff 组件不可用";
				return false;
			}
			object obj;
			if (configId <= 0)
			{
				obj = null;
			}
			else
			{
				ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
				obj = ((instance != null) ? instance.Get_Config_Buff(configId) : null);
			}
			if (obj == null)
			{
				LastError = "不存在的 Buff 配置";
				return false;
			}
			if (TryGetActiveSurvivalPlanByBuffId(configId, out var talentId))
			{
				return RemoveSurvivalPlan(talentId);
			}
			playerBuff.RemoveBuff(configId);
			try
			{
				playerBuff.RequestLeadingRoleBuffRefresh();
			}
			catch
			{
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "移除 Buff 失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool RemoveBuff(long instanceId)
	{
		WebUiBridge.SetOp("GameApi.RemoveBuff");
		try
		{
			BuffComponent playerBuff = PlayerBuff;
			if (playerBuff == null)
			{
				LastError = "Buff 组件不可用";
				return false;
			}
			if (instanceId <= 0)
			{
				LastError = "Buff 实例 ID 无效";
				return false;
			}
			System.Collections.Generic.List<BuffView> buffs = GetBuffs();
			for (int i = 0; i < buffs.Count; i++)
			{
				BuffView buffView = buffs[i];
				if (buffView.InstanceId == instanceId && TryGetActiveSurvivalPlanByBuffId(buffView.ConfigId, out var talentId))
				{
					return RemoveSurvivalPlan(talentId);
				}
			}
			playerBuff.RemoveBuff(instanceId);
			try
			{
				playerBuff.RequestLeadingRoleBuffRefresh();
			}
			catch
			{
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "移除 Buff 失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	private static HashSet<int> GetActiveSurvivalPlanBuffIds()
	{
		HashSet<int> hashSet = new HashSet<int>();
		SurvivalPlanningComponent playerSurvivalPlanning = PlayerSurvivalPlanning;
		List<int> val = ((playerSurvivalPlanning != null) ? playerSurvivalPlanning.SaveCache : null);
		if (val == null)
		{
			return hashSet;
		}
		var enumerator = val.GetEnumerator();
		while (enumerator.MoveNext())
		{
			Config_Talent val2 = FindTalentConfig(enumerator.Current);
			if (val2 != null && val2.BuffID > 0)
			{
				hashSet.Add(val2.BuffID);
			}
		}
		return hashSet;
	}

	private static bool TryGetActiveSurvivalPlanByBuffId(int configId, out int talentId)
	{
		talentId = 0;
		if (configId <= 0)
		{
			return false;
		}
		SurvivalPlanningComponent playerSurvivalPlanning = PlayerSurvivalPlanning;
		List<int> val = ((playerSurvivalPlanning != null) ? playerSurvivalPlanning.SaveCache : null);
		if (val == null)
		{
			return false;
		}
		var enumerator = val.GetEnumerator();
		while (enumerator.MoveNext())
		{
			int current = enumerator.Current;
			Config_Talent val2 = FindTalentConfig(current);
			if (val2 != null && val2.BuffID == configId)
			{
				talentId = current;
				return true;
			}
		}
		return false;
	}

	public static bool TryGetExposure(out float current, out int maximum, out float timeExposure, out float moveExposure, out bool running)
	{
		current = 0f;
		maximum = 0;
		timeExposure = 0f;
		moveExposure = 0f;
		running = false;
		try
		{
			ExploreManager explore = Explore;
			if (explore == null)
			{
				return false;
			}
			current = explore.CurExposure;
			maximum = explore.MaxExposure;
			timeExposure = explore.TimeExposure;
			moveExposure = explore.MoveExposure;
			running = explore.IsRunning;
			return true;
		}
		catch (Exception ex)
		{
			LogErr("读取暴露值失败: " + ex.Message);
			return false;
		}
	}

	public static bool SetExposure(float value)
	{
		WebUiBridge.SetOp("GameApi.SetExposure");
		try
		{
			ExploreManager explore = Explore;
			if (explore == null)
			{
				LastError = "探索管理器不可用（请进入探索场景后再试）";
				return false;
			}
			float num = explore.MaxExposure;
			if (num <= 0f)
			{
				LastError = "当前没有有效的暴露值上限";
				return false;
			}
			float curExposure_k__BackingField = Math.Clamp(value, 0f, num);
			explore._CurExposure_k__BackingField = curExposure_k__BackingField;
			explore.NotifyUI();
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "设置暴露值失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static int CurrentSurvivalPoints()
	{
		try
		{
			SurvivalResultsManager survivalResults = SurvivalResults;
			return (survivalResults != null) ? survivalResults.GetSurvivalPoint() : (-1);
		}
		catch
		{
			return -1;
		}
	}

	public static bool SetSurvivalPoints(int value)
	{
		WebUiBridge.SetOp("GameApi.SetSurvivalPoints");
		try
		{
			SurvivalResultsManager survivalResults = SurvivalResults;
			if (survivalResults == null)
			{
				LastError = "生存点管理器不可用";
				return false;
			}
			int survivalPoint = survivalResults.GetSurvivalPoint();
			survivalResults.AddSurvivalPoint(value - survivalPoint);
			int survivalPoint2 = survivalResults.GetSurvivalPoint();
			if (survivalPoint2 != value)
			{
				LastError = $"生存点修改未生效，当前为 {survivalPoint2}";
				return false;
			}
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "设置生存点失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static System.Collections.Generic.List<ProficiencyView> GetProficiencies()
	{
		System.Collections.Generic.List<ProficiencyView> list = new System.Collections.Generic.List<ProficiencyView>();
		try
		{
			ProficiencyManager proficiency = Proficiency;
			if (proficiency == null)
			{
				LastError = "熟练度管理器尚未加载";
				return list;
			}
			List<ProficiencySystemSnapshot> snapshot = proficiency.GetSnapshot();
			if (snapshot == null)
			{
				LastError = "未读取到熟练度数据";
				return list;
			}
			var enumerator = snapshot.GetEnumerator();
			while (enumerator.MoveNext())
			{
				ProficiencySystemSnapshot current = enumerator.Current;
				if (current != null && current.SystemId >= 1 && current.SystemId <= 6)
				{
					list.Add(new ProficiencyView
					{
						TypeId = current.SystemId,
						Name = current.SystemName,
						Level = current.CurrentLevel,
						Exp = current.CurrentExp,
						PreviousLevelExp = current.PrevLevelExp,
						NextLevelExp = current.NextLevelExp,
						MaxLevel = 5
					});
				}
			}
			// list.Sort((ProficiencyView left, ProficiencyView right) => left.TypeId.CompareTo(right.TypeId)); // il2cpp IComparer 不兼容
			LastError = "";
		}
		catch (Exception ex)
		{
			LastError = "读取熟练度失败: " + ex.Message;
			LogErr(LastError);
		}
		return list;
	}

	public static bool AddProficiencyExp(int typeId, int amount)
	{
		//IL_003c: Unknown result type (might be due to invalid IL or missing references)
		//IL_003e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0055: Unknown result type (might be due to invalid IL or missing references)
		WebUiBridge.SetOp("GameApi.AddProficiencyExp");
		try
		{
			if (typeId < 1 || typeId > 6 || amount <= 0)
			{
				LastError = "熟练度类型或经验数量无效";
				return false;
			}
			ProficiencyManager proficiency = Proficiency;
			if (proficiency == null)
			{
				LastError = "熟练度管理器尚未加载";
				return false;
			}
			ProficiencyType val = (ProficiencyType)typeId;
			if (proficiency.IsMaxLevel(val))
			{
				LastError = "该熟练度已达到最高等级";
				return false;
			}
			proficiency.AddExp(val, (string)null, amount, 0, 0, (ProficiencyExpSource)1);
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "增加熟练度经验失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	public static bool AddProficiencyLevels(int typeId, int levels, out int applied)
	{
		//IL_0042: Unknown result type (might be due to invalid IL or missing references)
		//IL_0044: Unknown result type (might be due to invalid IL or missing references)
		//IL_0070: Unknown result type (might be due to invalid IL or missing references)
		applied = 0;
		WebUiBridge.SetOp("GameApi.AddProficiencyLevels");
		try
		{
			if (typeId < 1 || typeId > 6 || levels <= 0)
			{
				LastError = "熟练度类型或等级数量无效";
				return false;
			}
			ProficiencyManager proficiency = Proficiency;
			if (proficiency == null)
			{
				LastError = "熟练度管理器尚未加载";
				return false;
			}
			ProficiencyType val = (ProficiencyType)typeId;
			int level = proficiency.GetLevel(val);
			applied = Math.Min(levels, Math.Max(0, 5 - level));
			if (applied <= 0)
			{
				LastError = "该熟练度已达到最高等级";
				return false;
			}
			proficiency.AddLevel(val, applied, false);
			LastError = "";
			return true;
		}
		catch (Exception ex)
		{
			LastError = "提升熟练度等级失败: " + ex.Message;
			LogErr(LastError);
			return false;
		}
		finally
		{
			WebUiBridge.SetOp(null);
		}
	}

	private static int GetNeighborAffinityMax()
	{
		int globalSetting_Neighbor_AffinityMax = GameKey.GlobalSetting_Neighbor_AffinityMax;
		if (globalSetting_Neighbor_AffinityMax <= 0)
		{
			return 100;
		}
		return globalSetting_Neighbor_AffinityMax;
	}

	public static bool SetMovementBlocked(bool blocked)
	{
		try
		{
			BattleShowWorld instance = BaseSingleton<BattleShowWorld>.Instance;
			if (instance == null)
			{
				return false;
			}
			CameraManager cameraManager = instance._CameraManager;
			if (cameraManager == null)
			{
				return false;
			}
			cameraManager.isKeyboardMoveBlocked = blocked;
			return true;
		}
		catch (Exception ex)
		{
			LogErr("SetMovementBlocked: " + ex.Message);
			return false;
		}
	}

	public static void SetHotKeyDisabled(bool disabled)
	{
		try
		{
			BattleShowWorld instance = BaseSingleton<BattleShowWorld>.Instance;
			HotKeyManager obj = ((instance != null) ? instance._HotKeyManager : null);
			if (obj != null)
			{
				obj.OnDisableResponse(disabled);
			}
		}
		catch (Exception ex)
		{
			LogErr("SetHotKeyDisabled: " + ex.Message);
		}
	}

	public static string Probe()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.AppendLine("[探测]");
		stringBuilder.AppendLine("World: " + ((World != null) ? "OK" : "NULL"));
		stringBuilder.AppendLine("ItemManager: " + ((Items != null) ? "OK" : "NULL"));
		stringBuilder.AppendLine("AgentManager: " + ((Agents != null) ? "OK" : "NULL"));
		stringBuilder.AppendLine("PlayerId: " + PlayerId);
		stringBuilder.AppendLine("LeadingRole: " + ((Player != null) ? "OK" : "NULL"));
		stringBuilder.AppendLine("UserComponent: " + ((PlayerUserComponent != null) ? "OK" : "NULL"));
		stringBuilder.AppendLine("AttributeComponent: " + ((PlayerAttributeComponent != null) ? "OK" : "NULL"));
		stringBuilder.AppendLine("Gold: " + CurrentGold());
		stringBuilder.AppendLine("Health(hidden): " + GetAttr((AttrName)4));
		stringBuilder.AppendLine("Vitality(HUD life): " + GetAttr((AttrName)5));
		stringBuilder.AppendLine("Stamina: " + GetAttr((AttrName)3));
		stringBuilder.AppendLine("Satiety: " + GetAttr((AttrName)1));
		stringBuilder.AppendLine("Morale: " + GetAttr((AttrName)2));
		return stringBuilder.ToString();
	}
}
