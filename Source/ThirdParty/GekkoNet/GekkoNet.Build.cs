using System.IO;
using UnrealBuildTool;

public class GekkoNet : ModuleRules
{
	public GekkoNet(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		bool bStaticLinking = false;
		bool bWithAsio = true;

		// Path to the submodule root
		string SubmoduleDir = Path.Combine(ModuleDirectory, "GekkoNet");
		string IncludeDir   = Path.Combine(SubmoduleDir, "GekkoLib", "include");
		string BinDir       = Path.Combine(ModuleDirectory, "Binaries");

		// Expose GekkoLib headers to dependents
		PublicIncludePaths.Add(IncludeDir);

		// Built with GEKKONET_STATIC — no DLL import/export needed
		if (bStaticLinking)
		{
			if (bWithAsio)
			{
				PublicDefinitions.Add("GEKKONET_STATIC");
			}
			else
			{
				PublicDefinitions.Add("GekkoNet_STATIC_NO_ASIO");
			}
		}

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string Win64BinDir = Path.Combine(BinDir, "Win64");
			if (bStaticLinking)
			{
				if (bWithAsio)
				{
					PublicAdditionalLibraries.Add(Path.Combine(Win64BinDir, "GekkoNet_STATIC.lib"));
				}
				else
				{
					PublicAdditionalLibraries.Add(Path.Combine(Win64BinDir, "GekkoNet_STATIC_NO_ASIO.lib"));
				}
			}
			else
			{
				// Add the import library
				PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Binaries", "Win64", "GekkoNet.lib"));

				// Delay-load the DLL, so we can load it from the right place first
				PublicDelayLoadDLLs.Add("GekkoNet.dll");

				// Ensure that the DLL is staged along with the executable
				RuntimeDependencies.Add("$(PluginDir)/Binaries/Win64/GekkoNet.dll");
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