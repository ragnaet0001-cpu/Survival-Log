namespace SurvivalLogCheat;

public class BackpackItemView
{
	public long InstanceId;

	public int ConfigId;

	public int Count;

	public float TimeScale;

	public string Name => ItemCatalog.GetName(ConfigId);

	public string IconPath => ItemCatalog.GetIconPath(ConfigId);
}
