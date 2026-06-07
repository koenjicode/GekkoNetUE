// If your sending data from one player to another using RPC's this controller is vital, other wise the code won't work as intended.

#pragma once

#include "CoreMinimal.h"
#include "gekkonet.h"
#include "GameFramework/PlayerController.h"
#include "GekkoNetRPCController.generated.h"

/**
 * 
 */
UCLASS()
class GEKKONETUE_API AGekkoNetRPCController : public APlayerController
{
	GENERATED_BODY()
	
public:
	// Send packets to another player through Unreal's RPCs.
	virtual void SendGekkoData(GekkoNetAddress* Addr, const char* Data, int Length);
protected:
	// Handles parsing the packet data through the server.
	UFUNCTION(Server, Reliable)
	virtual void Server_SendGekkoData(const FString& TargetAddress, const TArray<uint8>& Packet) const;
	// Handles what the client does when they recieve the packet information.
	UFUNCTION(Client, Reliable)
	virtual void Client_SendGekkoData(const TArray<uint8>& Packet);
	// Get the Sender's address from the sent packet.
	UFUNCTION()
	virtual FString GetRemoteAddressFromIdentifier(TArray<uint8> Packet);
public:
	// Handle the packets that have been received.
	virtual GekkoNetResult** ReceiveGekkoData(int* Length);
protected:
	// Received packets from other players.
	TQueue<TArray<uint8>> GekkoMessages;
	// The resulting data after being handled, this is sent back to the GekkoNet system!
	TArray<GekkoNetResult*> GekkoResults;
};
