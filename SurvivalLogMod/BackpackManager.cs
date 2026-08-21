using System;
using GameCore.HotUpdate;
using GameCore.HotUpdate.Battle.Logic;
using Il2CppInterop.Runtime.InteropTypes;
using Il2CppSystem.Collections.Generic;
using UnityEngine;

namespace SurvivalLogCheat;

internal static class BackpackManager
{
	private sealed class OriginalSize
	{
		public Config_Bag Config;

		public int Columns;

		public int Rows;
	}

	private const int MaximumExtraBurden = 100000000;

	private static readonly System.Collections.Generic.Dictionary<int, OriginalSize> OriginalSizes = new System.Collections.Generic.Dictionary<int, OriginalSize>();

	private static int _desiredColumns;

	private static int _desiredRows;

	private static int _desiredMaxBurden;

	private static IntPtr _burdenBagPointer;

	private static int _appliedExtraBurden;

	private static int _appliedTargetMaxBurden;

	private static int _appliedMaximum = -1;

	private static int _naturalMaxBurden = -1;

	private static BagComponent GetBag()
	{
		try
		{
			return (GameApi.Player == null) ? null : AgentTools.GetAgentComponent<BagComponent>((BaseAgent)(object)GameApi.Player);
		}
		catch
		{
			return null;
		}
	}

	internal static void SetRememberedSize(int columns, int rows)
	{
		if (columns >= 1 && columns <= 20 && rows >= 1 && rows <= 20)
		{
			_desiredColumns = columns;
			_desiredRows = rows;
		}
		else
		{
			_desiredColumns = 0;
			_desiredRows = 0;
		}
	}

	internal static void ApplySizeOverride(long ownerId, ref Vector2Int size)
	{
		if (_desiredColumns > 0 && _desiredRows > 0)
		{
			long playerId = GameApi.PlayerId;
			if (playerId != 0L && ownerId == playerId)
			{
				size.x = Math.Max(size.x, _desiredColumns);
				size.y = Math.Max(size.y, _desiredRows);
			}
		}
	}

	internal static void SetRememberedMaxBurden(int target)
	{
		_desiredMaxBurden = Math.Max(0, target);
	}

	public static bool TryGetInfo(out int columns, out int rows, out int weight, out int maxBurden, out int baseColumns, out int baseRows, out int naturalMaxBurden)
	{
		columns = (rows = (weight = (maxBurden = (baseColumns = (baseRows = (naturalMaxBurden = -1))))));
		BagComponent bag = GetBag();
		if (bag == null)
		{
			return false;
		}
		EnsureDesiredMaxBurden(bag);
		ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
		Config_Bag val = ((instance != null) ? instance.Get_Config_Bag(bag.GetBagConfigId()) : null);
		if (((val != null) ? val.Size : null) != null && val.Size.Count >= 2)
		{
			columns = val.Size[0];
			rows = val.Size[1];
			if (OriginalSizes.TryGetValue(bag.GetBagConfigId(), out var value))
			{
				baseColumns = value.Columns;
				baseRows = value.Rows;
			}
			else
			{
				baseColumns = columns;
				baseRows = rows;
			}
		}
		weight = bag.GetBagWeight();
		maxBurden = bag.GetMaxBurden();
		naturalMaxBurden = ((_naturalMaxBurden >= 0) ? _naturalMaxBurden : maxBurden);
		if (columns > 0)
		{
			return rows > 0;
		}
		return false;
	}

	public static bool SetSize(int columns, int rows, out string error)
	{
		return SetSizeCore(columns, rows, refreshUi: true, persistMemory: true, out error);
	}

	private static bool SetSizeCore(int columns, int rows, bool refreshUi, bool persistMemory, out string error)
	{
		error = null;
		if (columns < 1 || columns > 20 || rows < 1 || rows > 20)
		{
			error = "背包尺寸范围为 1 到 20";
			return false;
		}
		BagComponent bag = GetBag();
		if (bag == null)
		{
			error = "游戏背包尚未加载";
			return false;
		}
		int bagConfigId = bag.GetBagConfigId();
		ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
		Config_Bag val = ((instance != null) ? instance.Get_Config_Bag(bagConfigId) : null);
		if (((val != null) ? val.Size : null) == null || val.Size.Count < 2)
		{
			error = "未找到背包配置";
			return false;
		}
		if (!OriginalSizes.TryGetValue(bagConfigId, out var value))
		{
			value = new OriginalSize
			{
				Config = val,
				Columns = val.Size[0],
				Rows = val.Size[1]
			};
			OriginalSizes.Add(bagConfigId, value);
		}
		if (columns < value.Columns || rows < value.Rows)
		{
			error = "不能小于原始背包尺寸";
			return false;
		}
		if ((columns < val.Size[0] || rows < val.Size[1]) && !CanFitExistingItems(columns, rows, out error))
		{
			return false;
		}
		val.Size[0] = columns;
		val.Size[1] = rows;
		SetRememberedSize(columns, rows);
		if (persistMemory)
		{
			WebUiBridge.RememberBagSize(columns, rows, persist: true);
		}
		if (refreshUi)
		{
			RefreshUi();
		}
		return true;
	}

	public static bool ResetSize(out string error)
	{
		error = null;
		BagComponent bag = GetBag();
		if (bag == null)
		{
			error = "游戏背包尚未加载";
			return false;
		}
		int bagConfigId = bag.GetBagConfigId();
		ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
		Config_Bag val = ((instance != null) ? instance.Get_Config_Bag(bagConfigId) : null);
		if (((val != null) ? val.Size : null) == null || val.Size.Count < 2)
		{
			error = "未找到背包配置";
			return false;
		}
		int num = val.Size[0];
		int num2 = val.Size[1];
		if (OriginalSizes.TryGetValue(bagConfigId, out var value))
		{
			num = value.Columns;
			num2 = value.Rows;
		}
		if (!CanFitExistingItems(num, num2, out error))
		{
			return false;
		}
		val.Size[0] = num;
		val.Size[1] = num2;
		SetRememberedSize(0, 0);
		WebUiBridge.RememberBagSize(0, 0, persist: true);
		RefreshUi();
		return true;
	}

	public static bool SetMaxBurden(int target, out string error)
	{
		error = null;
		BagComponent bag = GetBag();
		if (bag == null)
		{
			error = "游戏背包尚未加载";
			return false;
		}
		TrackBurden(bag);
		int desiredMaxBurden = _desiredMaxBurden;
		RemoveAppliedExtraBurden(bag);
		int num = (_naturalMaxBurden = bag.GetMaxBurden());
		if (target < num)
		{
			RestoreTarget(bag, desiredMaxBurden, num);
			error = "不能小于当前原始最大负重";
			return false;
		}
		if (!ApplyTarget(bag, target, num, out error))
		{
			RestoreTarget(bag, desiredMaxBurden, num);
			return false;
		}
		_desiredMaxBurden = target;
		GameApi.LogInfo($"[负重] 已应用: 原始上限={num}, 目标={target}, 修改器补足={_appliedExtraBurden}, 实际上限={_appliedMaximum}");
		RefreshUi();
		return true;
	}

	public static void ResetMaxBurden()
	{
		BagComponent bag = GetBag();
		if (bag != null)
		{
			TrackBurden(bag);
			RemoveAppliedExtraBurden(bag);
			_naturalMaxBurden = bag.GetMaxBurden();
			_appliedMaximum = _naturalMaxBurden;
		}
		_desiredMaxBurden = 0;
		_appliedTargetMaxBurden = 0;
		RefreshUi();
	}

	public static bool AddItem(int configId, int count, out int added, out bool capped, out string error)
	{
		//IL_004b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0050: Unknown result type (might be due to invalid IL or missing references)
		//IL_00f6: Unknown result type (might be due to invalid IL or missing references)
		//IL_0135: Unknown result type (might be due to invalid IL or missing references)
		//IL_0102: Unknown result type (might be due to invalid IL or missing references)
		added = 0;
		capped = false;
		error = null;
		if (count <= 0)
		{
			error = "数量必须大于 0";
			return false;
		}
		ItemManager items = GameApi.Items;
		long playerId = GameApi.PlayerId;
		ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
		Config_Item val = ((instance != null) ? instance.Get_Config_Item(configId) : null);
		if (items == null || playerId == 0L || val == null)
		{
			error = "游戏数据尚未加载或物品不存在";
			return false;
		}
		Vector2Int itemSize = GetItemSize(val);
		int num = 0;
		try
		{
			BattleLogicWorld world = GameApi.World;
			int? obj;
			if (world == null)
			{
				obj = null;
			}
			else
			{
				GameTimeManager gameTimeManager = world._GameTimeManager;
				if (gameTimeManager == null)
				{
					obj = null;
				}
				else
				{
					ITimer timer = gameTimeManager.Timer;
					obj = ((timer != null) ? new int?(timer.GetTime()) : ((int?)null));
				}
			}
			int? num2 = obj;
			num = num2.GetValueOrDefault();
		}
		catch
		{
		}
		int num3 = (int)Math.Min((val.Life > 0) ? ((long)val.Life * 24L) : 9999999, 2147483647L);
		int val2 = ((val.StackLimit > 0) ? val.StackLimit : 9999);
		int num4 = count;
		while (num4 > 0)
		{
			if (!TryResolvePosition(items, playerId, itemSize, out var position) && !TryExpandFor(items, playerId, itemSize, out position))
			{
				error = ((added == 0) ? "背包没有可用空间，尺寸已达到 20 x 20" : "背包空间不足，已加入部分物品");
				break;
			}
			int num5 = Math.Min(num4, val2);
			if (items.AddItem(playerId, configId, num5, num, num3, position, false, false) == null)
			{
				error = ((added == 0) ? "加入物品失败" : "部分物品加入失败");
				break;
			}
			added += num5;
			num4 -= num5;
		}
		RefreshUi();
		return added > 0;
	}

	private static Vector2Int GetItemSize(Config_Item config)
	{
		//IL_0016: Unknown result type (might be due to invalid IL or missing references)
		//IL_0040: Unknown result type (might be due to invalid IL or missing references)
		if (config.Size == null || config.Size.Count < 2)
		{
			return Vector2Int.one;
		}
		return new Vector2Int(Math.Max(1, config.Size[0]), Math.Max(1, config.Size[1]));
	}

	private static bool TryResolvePosition(ItemManager items, long ownerId, Vector2Int itemSize, out Vector2Int position)
	{
		//IL_0001: Unknown result type (might be due to invalid IL or missing references)
		//IL_0009: Unknown result type (might be due to invalid IL or missing references)
		//IL_000c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0012: Unknown result type (might be due to invalid IL or missing references)
		position = default(Vector2Int);
		try
		{
			return items.TryResolveBagPos(ownerId, itemSize, default(Vector2Int), out position);
		}
		catch
		{
			return false;
		}
	}

	private static bool TryExpandFor(ItemManager items, long ownerId, Vector2Int itemSize, out Vector2Int position)
	{
		//IL_0001: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ab: Unknown result type (might be due to invalid IL or missing references)
		position = default(Vector2Int);
		if (!TryGetInfo(out var columns, out var rows, out var _, out var _, out var baseColumns, out var baseRows, out var _))
		{
			return false;
		}
		bool flag = false;
		for (int i = 0; i < 40; i++)
		{
			int val = ((columns < itemSize.x) ? itemSize.x : Math.Min(20, columns + 1));
			int val2 = ((rows < itemSize.y) ? itemSize.y : Math.Min(20, rows + 1));
			val = Math.Max(val, Math.Max(1, baseColumns));
			val2 = Math.Max(val2, Math.Max(1, baseRows));
			if ((val == columns && val2 == rows) || !SetSizeCore(val, val2, refreshUi: false, persistMemory: false, out var _))
			{
				break;
			}
			columns = val;
			rows = val2;
			flag = true;
			if (TryResolvePosition(items, ownerId, itemSize, out position))
			{
				WebUiBridge.RememberBagSize(columns, rows, persist: true);
				return true;
			}
		}
		if (flag)
		{
			WebUiBridge.RememberBagSize(columns, rows, persist: true);
		}
		return false;
	}

	private static bool CanFitExistingItems(int columns, int rows, out string error)
	{
		//IL_004f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0054: Unknown result type (might be due to invalid IL or missing references)
		//IL_0065: Unknown result type (might be due to invalid IL or missing references)
		//IL_006a: Unknown result type (might be due to invalid IL or missing references)
		//IL_009d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0096: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a2: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a6: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ab: Unknown result type (might be due to invalid IL or missing references)
		//IL_00c1: Unknown result type (might be due to invalid IL or missing references)
		//IL_00c6: Unknown result type (might be due to invalid IL or missing references)
		error = null;
		ItemManager items = GameApi.Items;
		long playerId = GameApi.PlayerId;
		if (items == null || playerId == 0L)
		{
			error = "游戏背包尚未加载";
			return false;
		}
		try
		{
			List<ItemData> itemDataList = items.GetItemDataList(playerId);
			if (itemDataList == null)
			{
				return true;
			}
			Il2CppSystem.Collections.Generic.List<ItemData>.Enumerator enumerator = itemDataList.GetEnumerator();
			while (enumerator.MoveNext())
			{
				ItemData current = enumerator.Current;
				if (current == null)
				{
					continue;
				}
				Vector2Int bagPos = current.BagPos;
				if (bagPos.x < 0)
				{
					continue;
				}
				bagPos = current.BagPos;
				if (bagPos.y < 0)
				{
					continue;
				}
				ConfigManager instance = BaseSingleton<ConfigManager>.Instance;
				Config_Item val = ((instance != null) ? instance.Get_Config_Item(current.ItemConfigId) : null);
				Vector2Int val2 = ((val == null) ? Vector2Int.one : GetItemSize(val));
				bagPos = current.BagPos;
				if (bagPos.x + val2.x <= columns)
				{
					bagPos = current.BagPos;
					if (bagPos.y + val2.y <= rows)
					{
						continue;
					}
				}
				error = "扩展区域还有物品，请先移到保留范围内再缩小背包";
				return false;
			}
			return true;
		}
		catch (Exception ex)
		{
			error = "检查背包物品位置失败: " + ex.Message;
			return false;
		}
	}

	private static void TrackBurden(BagComponent bag)
	{
		if (!(_burdenBagPointer == ((Il2CppObjectBase)bag).Pointer))
		{
			_burdenBagPointer = ((Il2CppObjectBase)bag).Pointer;
			_appliedExtraBurden = 0;
			_appliedTargetMaxBurden = 0;
			_naturalMaxBurden = bag.GetMaxBurden();
			_appliedMaximum = _naturalMaxBurden;
		}
	}

	private static void EnsureDesiredMaxBurden(BagComponent bag)
	{
		TrackBurden(bag);
		int maxBurden = bag.GetMaxBurden();
		if (_desiredMaxBurden <= 0)
		{
			if (_appliedExtraBurden > 0)
			{
				RemoveAppliedExtraBurden(bag);
				_naturalMaxBurden = bag.GetMaxBurden();
				_appliedMaximum = _naturalMaxBurden;
			}
		}
		else if (_appliedTargetMaxBurden != _desiredMaxBurden || maxBurden != _appliedMaximum)
		{
			RemoveAppliedExtraBurden(bag);
			int num = (_naturalMaxBurden = bag.GetMaxBurden());
			if (_desiredMaxBurden <= num)
			{
				_appliedTargetMaxBurden = _desiredMaxBurden;
				_appliedMaximum = num;
				return;
			}
			if (!ApplyTarget(bag, _desiredMaxBurden, num, out var error))
			{
				GameApi.LogErr("[负重] 自动恢复失败: " + error);
				return;
			}
			GameApi.LogInfo($"[负重] 已自动恢复: 原始上限={num}, 目标={_desiredMaxBurden}, 修改器补足={_appliedExtraBurden}, 实际上限={_appliedMaximum}");
		}
	}

	private static void RemoveAppliedExtraBurden(BagComponent bag)
	{
		if (_appliedExtraBurden > 0)
		{
			bag.RemoveExtraBurden(_appliedExtraBurden);
			_appliedExtraBurden = 0;
			_appliedTargetMaxBurden = 0;
		}
	}

	private static bool ApplyTarget(BagComponent bag, int target, int naturalMaximum, out string error)
	{
		error = null;
		if (target <= naturalMaximum)
		{
			_appliedTargetMaxBurden = target;
			_appliedMaximum = naturalMaximum;
			return true;
		}
		int num = MaximumWithExtra(bag, 100000000);
		if (target > num)
		{
			error = $"目标过大，当前最高可设置为 {num}";
			return false;
		}
		int num2 = FindClosestExtraBurden(bag, target, naturalMaximum);
		if (num2 > 0)
		{
			bag.AddExtraBurden(num2);
			_appliedExtraBurden = num2;
		}
		_appliedTargetMaxBurden = target;
		_appliedMaximum = bag.GetMaxBurden();
		return true;
	}

	private static void RestoreTarget(BagComponent bag, int target, int naturalMaximum)
	{
		if (target > naturalMaximum)
		{
			ApplyTarget(bag, target, naturalMaximum, out var _);
			return;
		}
		_appliedTargetMaxBurden = target;
		_appliedMaximum = naturalMaximum;
	}

	private static int FindClosestExtraBurden(BagComponent bag, int target, int naturalMaximum)
	{
		if (target <= naturalMaximum)
		{
			return 0;
		}
		int num = 0;
		int num2 = Math.Min(100000000, Math.Max(1, target - naturalMaximum));
		while (MaximumWithExtra(bag, num2) < target && num2 < 100000000)
		{
			num2 = Math.Min(100000000, num2 * 2);
		}
		while (num < num2)
		{
			int num3 = num + (num2 - num) / 2;
			if (MaximumWithExtra(bag, num3) < target)
			{
				num = num3 + 1;
			}
			else
			{
				num2 = num3;
			}
		}
		int num4 = MaximumWithExtra(bag, num);
		int num5 = ((num == 0) ? naturalMaximum : MaximumWithExtra(bag, num - 1));
		if (target - num5 > num4 - target)
		{
			return num;
		}
		return Math.Max(0, num - 1);
	}

	private static int MaximumWithExtra(BagComponent bag, int extra)
	{
		if (extra <= 0)
		{
			return bag.GetMaxBurden();
		}
		bag.AddExtraBurden(extra);
		try
		{
			return bag.GetMaxBurden();
		}
		finally
		{
			bag.RemoveExtraBurden(extra);
		}
	}

	private static void RefreshUi()
	{
		try
		{
			EventDispatcherTools.NotifyGlobal((EnumNotify_ItemBag)3027);
		}
		catch
		{
		}
	}
}
