using System;
using System.IO;
using System.Text;
using GameCore.HotUpdate;
using Il2CppSystem.Collections.Generic;
using UnityEngine;

namespace SurvivalLogCheat;

public static class ItemCatalog
{
	private static readonly System.Collections.Generic.List<ItemInfo> _items = new System.Collections.Generic.List<ItemInfo>();

	private static readonly System.Collections.Generic.Dictionary<int, ItemInfo> _byId = new System.Collections.Generic.Dictionary<int, ItemInfo>();

	private static readonly System.Collections.Generic.Dictionary<int, string> _subCatName = new System.Collections.Generic.Dictionary<int, string>();

	private static readonly System.Collections.Generic.List<TopCatInfo> _topCats = new System.Collections.Generic.List<TopCatInfo>();

	private static bool _refreshed;

	private static long _lastRefreshFrame;

	private static readonly System.Collections.Generic.Dictionary<int, string> _catNames = new System.Collections.Generic.Dictionary<int, string>
	{
		{ 0, "未分类" },
		{ 1, "食物" },
		{ 2, "药品" },
		{ 3, "书籍" },
		{ 4, "电器" },
		{ 5, "日用品" },
		{ 6, "工具" },
		{ 7, "宠物" },
		{ 8, "包裹" },
		{ 9, "材料" },
		{ 10, "种子" },
		{ 11, "肥料" },
		{ 12, "陷阱" },
		{ 13, "燃料" },
		{ 14, "背包家具" },
		{ 15, "花" },
		{ 16, "投掷物" }
	};

	public static System.Collections.Generic.List<ItemInfo> Items => _items;

	public static System.Collections.Generic.List<TopCatInfo> TopCategories => _topCats;

	public static ConfigManager Config
	{
		get
		{
			try
			{
				return BaseSingleton<ConfigManager>.Instance;
			}
			catch (Exception ex)
			{
				GameApi.LogErr("Config: " + ex.Message);
				return null;
			}
		}
	}

	public static bool IsReady
	{
		get
		{
			if (_refreshed)
			{
				return _items.Count > 0;
			}
			return false;
		}
	}

	public static void ResetCache()
	{
		_refreshed = false;
		_lastRefreshFrame = 0L;
		_items.Clear();
		_byId.Clear();
		_subCatName.Clear();
		_topCats.Clear();
	}

	public static void Refresh()
	{
		long num = Time.frameCount;
		if (_lastRefreshFrame > 0 && num - _lastRefreshFrame < 120)
		{
			return;
		}
		_lastRefreshFrame = num;
		try
		{
			ConfigManager config = Config;
			if (config == null)
			{
				return;
			}
			Dictionary<int, Config_Item> config_Item_Dict = config._Config_Item_Dict;
			if (config_Item_Dict == null)
			{
				return;
			}
			_subCatName.Clear();
			Dictionary<int, Config_ItemSubCategory> config_ItemSubCategory_Dict = config._Config_ItemSubCategory_Dict;
			if (config_ItemSubCategory_Dict != null)
			{
				Il2CppSystem.Collections.Generic.Dictionary<int, Config_ItemSubCategory>.Enumerator enumerator = config_ItemSubCategory_Dict.GetEnumerator();
				while (enumerator.MoveNext())
				{
					Config_ItemSubCategory value = enumerator.Current.Value;
					if (value != null)
					{
						string text = value.Name_Local ?? value.Name;
						_subCatName[value.ID] = (string.IsNullOrEmpty(text) ? ("分类" + value.ID) : text);
					}
				}
			}
			_items.Clear();
			_byId.Clear();
			Dictionary<int, Config_Item> val = config_Item_Dict;
			if (val != null)
			{
				Il2CppSystem.Collections.Generic.Dictionary<int, Config_Item>.Enumerator enumerator2 = val.GetEnumerator();
				while (enumerator2.MoveNext())
				{
					Config_Item value2 = enumerator2.Current.Value;
					if (value2 != null)
					{
						ItemInfo itemInfo = new ItemInfo
						{
							Id = value2.ID,
							Name = (value2.ItemName_Local ?? value2.ItemName ?? "未知"),
							Category = value2.Category,
							SubCategory = value2.SubCategory,
							Icon = (value2.Icon ?? ""),
							WebIcon = (value2.WebIcon ?? ""),
							Price = value2.price,
							Weight = value2.weight,
							TargetFurnitureID = value2.TargetFurnitureID
						};
						_items.Add(itemInfo);
						if (!_byId.ContainsKey(itemInfo.Id))
						{
							_byId[itemInfo.Id] = itemInfo;
						}
					}
				}
			}
			// _items.Sort((ItemInfo a, ItemInfo b) => a.Id.CompareTo(b.Id)); // il2cpp IComparer 不兼容，已禁用
			BuildTopCategories();
			_refreshed = true;
			GameApi.LogInfo($"[ItemCatalog] 目录刷新完成，共 {_items.Count} 种物品，{_topCats.Count} 个顶级分类");
		}
		catch (Exception ex)
		{
			GameApi.LogErr("ItemCatalog.Refresh: " + ex.Message);
		}
	}

	public static string GetName(int configId)
	{
		if (!_byId.TryGetValue(configId, out var value))
		{
			return "#" + configId;
		}
		return value.Name;
	}

	public static string GetSubCatName(int subCat)
	{
		if (!_subCatName.TryGetValue(subCat, out var value))
		{
			return "分类" + subCat;
		}
		return value;
	}

	private static void BuildTopCategories()
	{
		_topCats.Clear();
		SortedSet<int> sortedSet = new SortedSet<int>();
		foreach (ItemInfo item in _items)
		{
			if (item.Category > 0)
			{
				sortedSet.Add(item.Category);
			}
		}
		foreach (int item2 in sortedSet)
		{
			_topCats.Add(new TopCatInfo
			{
				Id = item2,
				Name = GetCategoryName(item2)
			});
		}
		StringBuilder stringBuilder = new StringBuilder();
		foreach (int item3 in sortedSet)
		{
			stringBuilder.Append(item3).Append('=').Append(GetCategoryName(item3))
				.Append("  ");
		}
		GameApi.LogInfo("[ItemCatalog] 顶级分类分布: " + stringBuilder.ToString().TrimEnd());
	}

	public static string GetCategoryName(int cat)
	{
		if (_catNames.TryGetValue(cat, out var value))
		{
			return value;
		}
		GameApi.LogErr("[ItemCatalog] 未识别的物品分类值: " + cat + "（不在官方枚举 0-16 内）");
		return "分类" + cat;
	}

	public static string GetIconPath(int configId)
	{
		if (_byId.TryGetValue(configId, out var value))
		{
			string icon = value.Icon;
			if (!string.IsNullOrEmpty(icon))
			{
				string text = BuildResPath(icon);
				if (text != null)
				{
					return text;
				}
			}
			if (!string.IsNullOrEmpty(value.WebIcon))
			{
				string text2 = BuildResPath(value.WebIcon);
				if (text2 != null)
				{
					return text2;
				}
			}
		}
		return null;
	}

	private static string BuildResPath(string icon)
	{
		if (string.IsNullOrEmpty(icon))
		{
			return null;
		}
		string text = icon.Replace('\\', '/').TrimStart('/');
		string[] array = new string[4] { ".webp", ".png", ".jpeg", ".jpg" };
		bool flag = false;
		string[] array2 = array;
		foreach (string value in array2)
		{
			if (text.EndsWith(value))
			{
				flag = true;
				break;
			}
		}
		string text2 = Path.Combine(Application.streamingAssetsPath, "WebUI", "Res");
		if (flag)
		{
			string text3 = Path.Combine(text2, text);
			if (File.Exists(text3))
			{
				return text3;
			}
		}
		string fileNameWithoutExtension = Path.GetFileNameWithoutExtension(text);
		try
		{
			array2 = Directory.GetDirectories(text2);
			foreach (string path in array2)
			{
				string[] array3 = array;
				foreach (string text4 in array3)
				{
					string text5 = Path.Combine(path, fileNameWithoutExtension + text4);
					if (File.Exists(text5))
					{
						return text5;
					}
				}
			}
		}
		catch
		{
		}
		return null;
	}
}
