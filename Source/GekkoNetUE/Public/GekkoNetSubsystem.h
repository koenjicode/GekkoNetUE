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
	UFUNCTION(BlueprintCallable)
	int32 AddActor(EGekkoPlayerType PlayerType = EGekkoPlayerType::LocalPlayer, FString Address = "");
	// Start the GekkoNet session based on the provided configuration and host.
	UFUNCTION(BlueprintCallable)
	void StartSession(FGekkoConfig InConfig, bool IsSpectator);
	// Shuts down an active GekkoNet session if running.
	UFUNCTION(BlueprintCallable)
	void EndSession();
	// Update GekkoNet related information and calls, whilst also updating the simulation state.
	UFUNCTION(BlueprintCallable)
	void UpdateSession();
	
	// Checks if a session is currently running.
	UFUNCTION(BlueprintPure)
	bool IsSessionRunning() const;
	// Get the current state of the session.
	UFUNCTION(BlueprintPure)
	EGekkoSessionState GetSessionState() const { return SessionState; };
	// Get the num of players allowed to be in the session.
	UFUNCTION(BlueprintPure)
	int32 GetNumOfPlayers() const { return Config.num_players; };
	// Return the simplified network stats collected from the active session.
	UFUNCTION()
	FGekkoSimpleNetworkStats UpdateNetworkStats(int32 Player);
	// Returns the advanced network stat information collected from the active session.
	UFUNCTION(BlueprintPure, DisplayName="Get Advanced Network Stats")
	FGekkoFullNetworkStats GetFullNetworkStats(int32 Player) const;
	
	void SetLocalPort(int32 NewLocalPort);
	// Set the type of socket that will be used to transfer data between connected clients.
	void SetTransportType(EGekkoTransportType Type);
	// Set the simulation host that will be used with GekkoNet.
	UFUNCTION(BlueprintCallable)
	bool SetSimulationHost(TScriptInterface<IGekkoNetSimulationInterface> NewHost);
	// Set a Player's input delay within the current session.
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelay(int32 Delay, int32 LocalPlayer);
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
	
#if WITH_EDITOR
	// Set the local adapter that will be used in PlayInEditor scenarios.
	void SetLocalAdapter(int32 Index);
	#endif
	

private:
	// subsystem functions
	
	void RunSession();
	
	void HandleDisconnection(GekkoSessionEvent* Ev);
	
	void StepLogic();
	void AddLocalInputs();

	void ProcessSession();
	void ProcessEvents();
	
	bool NeedToCatchUp() const;
	
#if WITH_EDITOR
	// Checks whether not the current session was created in PIE.
	bool IsPlayInEditor() const;
#endif
	
#if WITH_EDITORONLY_DATA
	bool bDisablePlayInEditorAdapters = false;
	int32 LocalAdapterID = 0;
#endif
	
	FGekkoSimpleNetworkStats NetStats;
	
	// session and stored session data
	
	GekkoSession* Session;
	EGekkoSessionState SessionState;
	EGekkoTransportType TransportType = EGekkoTransportType::Asio;
	
	TArray<int32> LocalPlayerIDs;
	int32 LocalPort = 7000;
	
	// frames skipping and frames behind
	
	int32 FrameMaxRollback = 0;
	int32 FrameSkipTimerMax = 60;
	int32 FramesAllowedToSkip = 1;
	int32 FrameSkipTimer = 0;
	int32 FramesBehind = 0;
	
	// delay and runahead
	
	int32 LocalDelay = DEFAULT_INPUT_DELAY;
	int32 LocalRunahead;
	
	// game simulation
	
	UPROPERTY()
	TScriptInterface<IGekkoNetSimulationInterface> SimHost;
	TArray<uint8> LocalInputBuffer;
	GekkoConfig Config;
};
