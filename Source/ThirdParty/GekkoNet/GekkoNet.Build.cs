using System.IO;
using UnrealBuildTool;

public class GekkoNet : ModuleRules
{
	public GekkoNet(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		bool bWithAsio = true;

		// Path to the submodule root
		string SubmoduleDir = Path.Combine(ModuleDirectory, "GekkoNet");
		string IncludeDir   = Path.Combine(SubmoduleDir, "GekkoLib", "include");
		string BinDir       = Path.Combine(ModuleDirectory, "Binaries");

		// Expose GekkoLib headers to dependents
		PublicIncludePaths.Add(IncludeDir);

		// Built with GEKKONET_STATIC — no DLL import/export needed
		if (bWithAsio)
		{
			PublicDefinitions.Add("GEKKONET_STATIC");
		}
		else
		{
			PublicDefinitions.Add("GekkoNet_STATIC_NO_ASIO");
		}

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string Win64BinDir = Path.Combine(BinDir, "Win64");
			if (bWithAsio)
			{
				PublicAdditionalLibraries.Add(Path.Combine(Win64BinDir, "GekkoNet_STATIC.lib"));
			}
			else
			{
				PublicAdditionalLibraries.Add(Path.Combine(Win64BinDir, "GekkoNet_STATIC_NO_ASIO.lib"));
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string LinuxBinDir = Path.Combine(BinDir, "Linux");
			if (bWithAsio)
			{
				PublicAdditionalLibraries.Add(Path.Combine(LinuxBinDir, "libGekkoNet_STATIC.a"));
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string MacBinDir = Path.Combine(BinDir, "Mac");
			if (bWithAsio)
			{
				PublicAdditionalLibraries.Add(Path.Combine(MacBinDir, "libGekkoNet_STATIC.a"));
			}
		}
	}
}