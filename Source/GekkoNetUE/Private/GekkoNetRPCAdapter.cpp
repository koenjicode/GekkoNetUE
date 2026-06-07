// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoNetRPCAdapter.h"
#include "GekkoNetLog.h"
#include "GekkoNetRPCController.h"

TWeakObjectPtr<AGekkoNetRPCController> RPCController;

void RPC_Send(GekkoNetAddress* Addr, const char* Data, int Length)
{
	if (!RPCController.IsValid())
	{
		return;
	}
	
	RPCController->SendGekkoData(Addr, Data, Length);
}

GekkoNetResult** RPC_Receive(int* Length)
{
	if (!RPCController.IsValid())
	{
		return nullptr;
	}
	
	return RPCController->ReceiveGekkoData(Length);
}

void RPC_Free(void* Data_Ptr)
{
	FMemory::Free(Data_Ptr);
}

static GekkoNetAdapter GekkoRPCAdapter
{
	RPC_Send,
	RPC_Receive,
	RPC_Free
};

GekkoNetAdapter* FGekkoNetRPCAdapter::RPC_Gekko_Adapter(APlayerController* InController)
{
	
	if (RPCController.IsValid())
	{
		UE_LOG(LogGekkoNet, Warning, TEXT("RPC Controller is already valid, please run not in one process!"));
		RPCController = nullptr;
	}
	
	RPCController = Cast<AGekkoNetRPCController>(InController);
	
	if (!RPCController.IsValid())
	{
		UE_LOG(LogGekkoNet, Error, TEXT("Failed to create RPC Gekko Adapter"));
		return nullptr;
	}
	
	return &GekkoRPCAdapter;
}
