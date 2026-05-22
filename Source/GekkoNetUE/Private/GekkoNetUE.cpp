// Copyright Epic Games, Inc. All Rights Reserved.

#include "GekkoNetUE.h"
#include "GekkoNetLog.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGekkoNet);

IMPLEMENT_MODULE(FGekkoNetUEModule, GekkoNetUE)

void FGekkoNetUEModule::StartupModule()
{
	
	FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("GekkoNetUE"))->GetBaseDir();
	FString DllPath = FPaths::Combine(BaseDir, TEXT("Binaries/Win64/GekkoNet.dll"));

	GekkoNetHandle = FPlatformProcess::GetDllHandle(*DllPath);

	if (!GekkoNetHandle)
	{
		UE_LOG(LogGekkoNet, Error, TEXT("GekkoNetUE: Failed to load GekkoNet.dll"));
	}
	else
	{
		UE_LOG(LogGekkoNet, Log, TEXT("GekkoNetUE: Loaded GekkoNet.dll"));
	}
}

void FGekkoNetUEModule::ShutdownModule()
{
	// Free the dll handle
	FPlatformProcess::FreeDllHandle(GekkoNetHandle);
	GekkoNetHandle = nullptr;
	
	UE_LOG(LogTemp, Log, TEXT("GekkoNetUE: Module shutdown."));
}