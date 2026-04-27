using System.IO;
using UnrealBuildTool;

public class GekkoNet : ModuleRules
{
	public GekkoNet(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		// Path to the submodule root
		string SubmoduleDir = Path.Combine(ModuleDirectory, "GekkoNet");
		string IncludeDir   = Path.Combine(SubmoduleDir, "GekkoLib", "include");
		string BinDir       = Path.Combine(ModuleDirectory, "Binaries");

		// Expose GekkoLib headers to dependents
		PublicIncludePaths.Add(IncludeDir);

		// Built with GEKKONET_STATIC — no DLL import/export needed
		PublicDefinitions.Add("GEKKONET_STATIC");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string Win64BinDir = Path.Combine(BinDir, "Win64");

			PublicAdditionalLibraries.Add(Path.Combine(Win64BinDir, "GekkoNet_STATIC.lib"));

			// ws2_32 required by ASIO on Windows
			PublicSystemLibraries.Add("ws2_32.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string LinuxBinDir = Path.Combine(BinDir, "Linux");
			PublicAdditionalLibraries.Add(Path.Combine(LinuxBinDir, "libGekkoNet_STATIC.a"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string MacBinDir = Path.Combine(BinDir, "Mac");
			PublicAdditionalLibraries.Add(Path.Combine(MacBinDir, "libGekkoNet_STATIC.a"));
		}
	}
}