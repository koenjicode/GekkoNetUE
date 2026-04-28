// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "gekkonet.h"

class FGekkoNetAdapter
{
public:
	static GekkoNetAdapter* UE_Gekko_Adapter(int32 Port);
	static void UE_Gekko_Adapter_Destroy();
};
