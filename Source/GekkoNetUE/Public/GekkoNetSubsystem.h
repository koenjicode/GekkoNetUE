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
	// Start the GekkoNet session based on the provided configuration and host.
	UFUNCTION(BlueprintCallable)
	void StartGekko(FGekkoSessionConfig Config, TScriptInterface<IGekkoNetSimulationInterface> NewHost, int32 PlayerIndex = -1);
	void StartGekko(FGekkoSessionConfig Config, int32 PlayerIndex = -1);
	// Shuts down an active GekkoNet session if running.
	UFUNCTION(BlueprintCallable)
	void ShutdownGekko();
	// Update GekkoNet related information and calls, whilst also updating the simulation state.
	UFUNCTION(BlueprintCallable)
	void UpdateGekko();
	
	// Checks if a session is currently running.
	UFUNCTION(BlueprintPure)
	bool IsSessionActive() const { return Session != nullptr; }
	// Get the current state of the session.
	UFUNCTION(BlueprintPure)
	EGekkoSessionState GetSessionState() const { return SessionState; };
	// Get the num of players allowed to be in the session.
	UFUNCTION(BlueprintPure)
	int32 GetNumOfPlayers() const { return NumPlayers; };
	// Return the simplified network stats collected from the active session.
	UFUNCTION(BlueprintPure)
	FGekkoSimpleNetworkStats GetNetworkStats() const { return NetStats; }
	// Returns the advanced network stat information collected from the active session.
	UFUNCTION(BlueprintPure)
	FGekkoFullNetworkStats GetFullNetworkStats() const;
	
	// Set the ID of the player that is connecting to the session.
	UFUNCTION(BlueprintCallable)
	void SetPlayerID(int32 NewID);
	// Set the simulation host that will be used with GekkoNet.
	UFUNCTION(BlueprintCallable)
	bool SetSimulationHost(TScriptInterface<IGekkoNetSimulationInterface> NewHost);
	// Set a Player's input delay within the current session.
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelay(int32 LocalPlayer, int32 Delay = 1);
	// Set how many frames ahead the simulation should run.
	UFUNCTION(BlueprintCallable)
	bool SetRunahead(int32 Runahead = 1);
	
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnPlayerConnected;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnPlayerDisconnected;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoSyncingEvent OnPlayerSyncing;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoSessionStartedEvent OnSessionStarted;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnSpectatorPaused;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoPlayerEvent OnSpectatorUnpaused;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") FGekkoDesyncEvent OnDesyncDetected;
	
private:
	// subsystem functions
	
	void RunNetplay();
	
	void HandleDisconnection(GekkoSessionEvent* Ev);
	
	void StepLogic(bool bShouldDraw);
	
	void ProcessSession();
	void ProcessEvents(bool bShouldDraw);
	
	void UpdateNetworkStats();
	
	bool NeedToCatchUp() const;
    
	// game player index
	
	int32 PlayerID = INDEX_NONE;
	int32 PlayerHandle = INDEX_NONE;
	
	// network stats
	
	FGekkoSimpleNetworkStats NetStats;
	
	int32 StatsUpdateTimer;
	int32 FrameMaxRollback;
	
	// local networking
	// TODO: store remoteports as remote addresses to better accomodate for multiple players entering.
	
	int32 LocalPort = INDEX_NONE;
	int32 RemotePort = INDEX_NONE;
	
	// session and stored session data
	
	GekkoSession* Session;
	EGekkoSessionState SessionState;
	
	int32 NumPlayers;
	
	int32 InputSize;
	int32 StateSize;
	
	// frames skipping and frames behind
	
	int32 FrameSkipTimer;
	int32 FramesBehind;
	int32 TransitionReadyFrames;
	
	// delay and runahead
	
	int32 LocalDelay = DEFAULT_INPUT_DELAY;
	int32 LocalRunahead = 0;
	
	// game simulation
	
	UPROPERTY()
	TScriptInterface<IGekkoNetSimulationInterface> SimHost;
};
