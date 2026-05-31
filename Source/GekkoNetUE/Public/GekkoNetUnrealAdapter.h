// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "gekkonet.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

class FGekkoNetAdapter
{
public:
	static GekkoNetAdapter* UE_Gekko_Adapter(FIPv4Endpoint Endpoint);
	static void UE_Gekko_Adapter_Destroy();
};
