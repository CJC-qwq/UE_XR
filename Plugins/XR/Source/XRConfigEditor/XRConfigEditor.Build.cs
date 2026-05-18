using UnrealBuildTool;

public class XRConfigEditor : ModuleRules
{
	public XRConfigEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"XR"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
				"EditorStyle",
				"LevelEditor",
				"PropertyEditor",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd"
			});
	}
}
