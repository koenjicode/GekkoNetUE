// Copyright Epic Games, Inc. All Rights Reserved.

#include "GekkoNetUE.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FGekkoNetUEModule, GekkoNetUE)

void FGekkoNetUEModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("GekkoNetUE: Module started."));
}

void FGekkoNetUEModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("GekkoNetUE: Module shutdown."));
}