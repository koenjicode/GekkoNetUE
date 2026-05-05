// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "gekkonet.h"
#include "GekkoNetTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GekkoNetSubsystem.generated.h"

#define GEKKO_DEBUG
#undef UNREAL_SOCKETS

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGekkoPlayerEvent, int32, Handle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGekkoSyncingEvent, int32, Handle, int32, Current, int32, Max);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGekkoSessionStartedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGekkoDesyncEvent, FGekkoDesyncInfo, Info);

/**
 * 
 */
UCLASS()
class GEKKONETUE_API UGekkoNetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
    
public:
	
	void StartGekko(FGekkoSessionConfig config);
	void ShutdownGekko();

	void UpdateNetplay();
	void RunNetplay();
	
	void HandleDisconnection(GekkoSessionEvent* Ev);
	
	void StepLogic(bool bShouldDraw);
	
	void ProcessSession();
	void ProcessEvents(bool bShouldDraw);
	
	void UpdateNetworkStats();
	
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelay(int32 LocalPlayer, int32 Delay);
	UFUNCTION(BlueprintCallable)
	bool SetRunahead(int32 Runahead);
	
	bool NeedToCatchUp() const;
    
	// game player index
	int32 PlayerNumber = INDEX_NONE;
	int32 PlayerHandle = INDEX_NONE;
	
	
	// network stats
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGekkoNetworkStats NetStats;
	int32 StatsUpdateTimer;
	int32 FrameMaxRollback;
	
	// local networking
	int32 LocalPort = INDEX_NONE;
	int32 RemotePort = INDEX_NONE;
	
	// session
	GekkoSession* Session;
	EGekkoSessionState SessionState;
	int32 FrameSkipTimer;
	int32 FramesBehind;
	int32 TransitionReadyFrames;
	
	
	int32 NumPlayers;
	int32 InputSize;
	int32 StateSize;
	
	// simulation state
	UPROPERTY()
	TScriptInterface<IGekkoNetSimulationInterface> SimulationHost;
	
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnPlayerConnected;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnPlayerDisconnected;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoSyncingEvent OnPlayerSyncing;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoSessionStartedEvent OnSessionStarted;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnSpectatorPaused;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnSpectatorUnpaused;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoDesyncEvent OnDesyncDetected;
};
