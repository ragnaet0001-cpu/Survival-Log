namespace SurvivalLogCheat;

public sealed class BuffView
{
	public long InstanceId { get; set; }

	public int ConfigId { get; set; }

	public string Name { get; set; } = "未知 Buff";

	public bool IsGood { get; set; }

	public int Layers { get; set; }

	public int TimeEndTime { get; set; }

	public bool RemoveByConfig { get; set; }

	public string Source { get; set; } = "当前生效";
}
