// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoNetRPCController.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void AGekkoNetRPCController::SendGekkoData(GekkoNetAddress* Addr, const char* Data, int Length)
{
	// This is a relatively open implementation that can be
	int32 Sender = PlayerState->GetPlayerId();
	
	TArray<uint8> Packet;
	Packet.AddUninitialized(sizeof(int32));
	FMemory::Memcpy(Packet.GetData(), &Sender, sizeof(int32));
	
	Packet.Append((uint8*)Data, Length);
	
	FString GekkoAddress = FString(UTF8_TO_TCHAR((const char*)Addr->data));
	Server_SendGekkoData(GekkoAddress, Packet);
}

void AGekkoNetRPCController::Server_SendGekkoData_Implementation(const FString& TargetAddress, const TArray<uint8>& Packet) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGekkoNetRPCController* PC = Cast<AGekkoNetRPCController>(*It);

		if (PC->PlayerState->GetUniqueId().ToString() == TargetAddress)
		{
			PC->Client_SendGekkoData(Packet);
		}
	}
}

void AGekkoNetRPCController::Client_SendGekkoData_Implementation(const TArray<uint8>& Packet)
{
	GekkoMessages.Enqueue(Packet);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Data sent to %s from %s"), *PlayerState->GetUniqueId().ToString(), *GetRemoteAddressFromIdentifier(Packet));
}

GekkoNetResult** AGekkoNetRPCController::ReceiveGekkoData(int* Length)
{
	GekkoResults.Reset();
	TArray<uint8> Packet;

	while (GekkoMessages.Dequeue(Packet))
	{
		int32 SenderSize = sizeof(int32);

		GekkoNetResult* Result = static_cast<GekkoNetResult*>(FMemory::Malloc(sizeof(GekkoNetResult)));
		FMemory::Memzero(Result, sizeof(GekkoNetResult));

		Result->data_len = Packet.Num() - SenderSize;
		Result->data = FMemory::Malloc(Result->data_len);
		
		FMemory::Memcpy(Result->data, Packet.GetData() + SenderSize, Result->data_len);
		
		FString AddrStr = GetRemoteAddressFromIdentifier(Packet);
		FTCHARToUTF8 Convert(AddrStr);

		Result->addr.size = Convert.Length();
		Result->addr.data = FMemory::Malloc(Result->addr.size);

		FMemory::Memcpy(Result->addr.data, Convert.Get(), Result->addr.size);
		GekkoResults.Add(Result);
	}

	*Length = GekkoResults.Num();
	return GekkoResults.GetData();
}

FString AGekkoNetRPCController::GetRemoteAddressFromIdentifier(TArray<uint8> Packet)
{
	int32 Sender = 0;
	FMemory::Memcpy(&Sender, Packet.GetData(), sizeof(int32));
	
	const auto PlayerArray = GetWorld()->GetGameState()->PlayerArray;
	for (int i = 0; i < PlayerArray.Num(); ++i)
	{
		if (PlayerArray[i]->GetPlayerId() == Sender)
		{
			return PlayerArray[i]->GetUniqueId().ToString();
		}
	}
	return "NULL";
	
}
