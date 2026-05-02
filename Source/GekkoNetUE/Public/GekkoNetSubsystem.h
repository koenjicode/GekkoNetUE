// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "gekkonet.h"
#include "GekkoNetTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GekkoNetSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class GEKKONETUE_API UGekkoNetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
    
public:
    
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Creates a new GekkoNet online session that allows players to connect together. A session config needs to be provided.
	UFUNCTION(BlueprintCallable)
	bool CreateSession(int32 LocalPort = 7000, bool AsSpectator = false);
	// Destroys the currrent GekkoNet session that is running.
	UFUNCTION(BlueprintCallable)
	void DestroySession();
	
	// TODO: Find a way to pass this through in blueprints, might not be possible.
	bool AddLocalInput(int32 PlayerIndex, void* Input);
	
	// Set a new session configuration, this session config will be used next time.
	UFUNCTION(BlueprintCallable)
	void SetSessionConfig(FGekkoSessionConfig NewConfig);
	// Sets the local delay for an existing local player in the game.
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelay(int32 PlayerIndex, int32 Delay);
	// Sets the same local delay for all local players present in the game.
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelayForAllPlayers(int32 Delay);
	// Set how many frames the game should process in advance.
	UFUNCTION(BlueprintCallable)
	bool SetRunahead(int32 Runahead);
	
	// Checks whether a session is currently running.
	UFUNCTION(BlueprintPure)
	bool IsSessionActive() const { return Session != nullptr; }
	// Get how many frames difference between you and other clients. Should be used within deterministic loop calculations.
	UFUNCTION(BlueprintPure)
	float GetFramesAhead() const;
	
	// Get network stats in association from a player.
	UFUNCTION(BlueprintPure)
	bool GetNetworkStats(int32 PlayerIndex, FGekkoNetworkStats& OutStats) const;
	// Get a player's ping.
	UFUNCTION(BlueprintPure)
	float GetPlayerPing(int32 PlayerIndex) const;
	// Checks if the specified player is of a specified type.
	UFUNCTION(BlueprintPure)
	bool IsPlayerType(EGekkoPlayerType Type, int32 PlayerIndex);
	// Checks if the player is local.
	UFUNCTION(BlueprintPure)
	bool IsLocalPlayer(int32 PlayerIndex);
	// Checks if the player is remote.
	UFUNCTION(BlueprintPure)
	bool IsRemotePlayer(int32 PlayerIndex);
	// Checks if the player is a spectator.
	UFUNCTION(BlueprintPure)
	bool IsSpectator(int32 PlayerIndex);
    
	// The Associated player ID that will be used when connecting.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 PlayerId;
	// Current Session Config that is stored in the subsystem.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGekkoSessionConfig Config;
	// GekkoNet session.
	GekkoSession* Session;
};
