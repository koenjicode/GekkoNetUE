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
	
	// Set a new session configuration, this session config will be used next time.
	UFUNCTION(BlueprintCallable)
	void SetSessionConfig(FGekkoSessionConfig NewConfig);
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelay(int32 PlayerIndex, int32 Delay);
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelayForAllPlayers(int32 Delay);
	UFUNCTION(BlueprintCallable)
	bool SetRunahead(int32 Runahead);
	
	
	UFUNCTION(BlueprintPure)
	bool IsSessionActive() const { return Session != nullptr; }
	UFUNCTION(BlueprintPure)
	float GetFramesAhead() const;
	
	UFUNCTION(BlueprintPure)
	bool GetNetworkStats(int32 PlayerIndex, FGekkoNetworkStats& OutStats) const;
	UFUNCTION(BlueprintPure)
	float GetPlayerPing(int32 PlayerIndex) const;
	UFUNCTION(BlueprintPure)
	bool IsPlayerType(EGekkoPlayerType Type, int32 PlayerIndex);
	UFUNCTION(BlueprintPure)
	bool IsLocalPlayer(int32 PlayerIndex);
	UFUNCTION(BlueprintPure)
	bool IsRemotePlayer(int32 PlayerIndex);
	UFUNCTION(BlueprintPure)
	bool IsSpectator(int32 PlayerIndex);
    
	// Current Session Config that is stored in the subsystem.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGekkoSessionConfig Config;
	// GekkoNet session.
	GekkoSession* Session;
};
