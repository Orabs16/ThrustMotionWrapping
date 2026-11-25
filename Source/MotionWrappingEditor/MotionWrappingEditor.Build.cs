using UnrealBuildTool;

public class MotionWrappingEditor : ModuleRules
{
	public MotionWrappingEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetTools", "MotionWrapping" });
	}
}
