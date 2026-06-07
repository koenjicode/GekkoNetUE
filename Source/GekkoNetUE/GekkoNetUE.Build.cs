using UnrealBuildTool;

public class GekkoNetUE : ModuleRules
{
	public GekkoNetUE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GekkoNet",
			"Projects",
			"SteamSockets",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"CoreOnline",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Sockets",
			"Networking"
		});
		
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd"
				}
			);
		}
	}
}