#pragma once

#include "CoreMinimal.h"
#include "gekkonet.h"
#include "GekkoNetTypes.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
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
	void CreateAdapter() const;
	// Start the GekkoNet session based on the provided configuration and host.
	UFUNCTION(BlueprintCallable)
	void StartSession(FGekkoConfig InConfig, EGekkoSessionType SessionType = EGekkoSessionType::Game);
	void DestroyAdapter() const;
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
	int32 GetNumOfPlayers() const { return Config.num_players; }
	UFUNCTION()
	void UpdateNetworkStats();
	UFUNCTION(BlueprintPure)
	FGekkoNetworkStats GetNetworkStats(int32 Index);
	// Returns the advanced network stat information collected from the active session.
	UFUNCTION(BlueprintPure, DisplayName="Get Advanced Network Stats")
	FGekkoAdvancedNetworkStats GetFullNetworkStats(int32 Player) const;
	
	GekkoSessionType SessionType = GekkoGameSession;

	bool SetLocalEndpoint(FString InEndpointString);
	bool SetLocalAddress(FString InAddress);
	void SetLocalPort(int32 InLocalPort);
	// Set the type of socket that will be used to transfer data between connected clients.
	void SetTransportType(EGekkoTransportType Type);
	// Set the simulation host that will be used with GekkoNet.
	UFUNCTION(BlueprintCallable)
	bool SetSimulationHost(TScriptInterface<IGekkoNetSimulationInterface> NewHost);
	// Set a Player's input delay within the current session.
	bool SetLocalDelay(int32 Delay, bool AdjustWithRunahead = true);
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelay(int32 Delay, int32 LocalPlayer, bool AdjustWithRunahead = true);
	// Set how many frames ahead the simulation should run.
	UFUNCTION(BlueprintCallable)
	bool SetRunahead(int32 Runahead = 1);
	
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") 
	FGekkoPlayerEvent OnPlayerConnected;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") 
	FGekkoPlayerEvent OnPlayerDisconnected;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") 
	FGekkoSyncingEvent OnPlayerSyncing;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") 
	FGekkoSessionStartedEvent OnSessionStarted;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") 
	FGekkoPlayerEvent OnSpectatorPaused;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") 
	FGekkoPlayerEvent OnSpectatorUnpaused;
	UPROPERTY(BlueprintAssignable, Category = "GekkoNet|Events") 
	FGekkoDesyncEvent OnDesyncDetected;
	
	int32 NetStatsUpdateTimer;

#if WITH_EDITOR
	// Set the local adapter that will be used in PlayInEditor scenarios.
	void SetLocalAdapter(int32 Index);
	#endif
	
	// Whether direct P2P adapters will be used over the RPC adapter implementation.
	bool bUseDirectAdapterIfAvailable = false;
	// Whether to use asio transport that comes with GekkoNet instead of the NULL subsystem socket.
	bool bUseAsioTransport = false;
	// How long before network stats are updated.
	// this is ticked based on the frames of your game loop, so it's best to ensure it ticks at least one full second!
	int32 FramesBeforeNextStatsUpdate = 60;
	

private:
	// subsystem functions
	
	void RunSession();
	
	void HandleDisconnection(GekkoSessionEvent* Ev);
	
	void StepLogic();
	void AddLocalInputs();

	void ProcessSession();
	void ProcessEvents();
	
	bool NeedToCatchUp() const;
	
	TArray<int32> LocalPlayers;
	
#if WITH_EDITOR
	// Checks whether not the current session was created in PIE.
	bool IsPlayInEditor() const;
#endif
	
#if WITH_EDITORONLY_DATA
	bool bDisablePlayInEditorAdapters = false;
	int32 LocalAdapterID = 0;
#endif
	
	// session and stored session data
	
	GekkoSession* Session;
	EGekkoSessionState SessionState;
	
	TArray<int32> LocalPlayerIDs;
	FGekkoEndpoint LocalEndpoint;
	
	// frames skipping and frames behind
	
	int32 MaxRollbackFrames = 0;
	int32 FrameSkipTimerMax = 60;
	int32 FramesAllowedToSkip = 1;
	int32 FrameSkipTimer = 0;
	int32 FramesBehind = 0;
	
protected:
	
	// delay and runahead
	
	UPROPERTY(BlueprintReadOnly)
	int32 LocalDelay;
	UPROPERTY(BlueprintReadOnly)
	int32 LocalRunahead;
	UPROPERTY(BlueprintReadOnly)
	TArray<FGekkoNetworkStats> NetStats;
	
private:
	
	UPROPERTY()
	TScriptInterface<IGekkoNetSimulationInterface> SimHost;
	TArray<uint8> LocalInputBuffer;
	
	GekkoConfig Config = {};
};
