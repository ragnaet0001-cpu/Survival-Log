namespace SurvivalLogCheat;

public sealed class SurvivalPlanView
{
	public int TalentId { get; set; }

	public string Name { get; set; } = "未命名生存规划";

	public string Description { get; set; } = "";

	public int Level { get; set; }

	public bool Active { get; set; }
}
