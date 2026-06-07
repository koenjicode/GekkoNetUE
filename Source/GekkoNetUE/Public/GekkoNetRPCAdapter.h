// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "gekkonet.h"

class FGekkoNetRPCAdapter
{
public:
	static GekkoNetAdapter* RPC_Gekko_Adapter(APlayerController* InController);
};
