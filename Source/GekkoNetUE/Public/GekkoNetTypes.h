#pragma once

#include "CoreMinimal.h"
#include "GekkoNetTypes.generated.h"

#define DEFAULT_INPUT_DELAY 1
#define MAX_INPUT_DELAY 8

UENUM()
enum class EGekkoSessionState : uint8
{
	Inactive,
	Running,
};

// The transport method used for the GekkoNet.
// It's worth noting that these transport methods only take effect in standalone builds of the game.
// PIE Adapter builds of the game will use a specialised LocalAdapter to facilitate communication.
UENUM()
enum class EGekkoTransportType : uint8
{
	// Uses the default adapter that comes with GekkoNet.
	Asio UMETA(DisplayName = "Use Asio Sockets"),
	// Uses an Unreal based adapter that was created for GekkoNetUE, useful if building with ASIO is disabled.
	Unreal UMETA(DisplayName = "Use Unreal Sockets"),
};

UENUM(BlueprintType)
enum class EGekkoPlayerType : uint8
{
	LocalPlayer,
	RemotePlayer,
	Spectator,
};

UENUM(BlueprintType)
enum class EGekkoSessionType : uint8
{
	// A standard GekkoNet session that used between connecting clients.
	Game UMETA(DisplayName = "Game Session"),
	// A session type that spectates an on-going game session that's played between clients.
	Spectator UMETA(DisplayName = "Spectator Session"),
	// A special session type that can be used to find desyncs in a game state.
	Stress UMETA(DisplayName = "Stress Session"),
};

USTRUCT(BlueprintType)
struct FGekkoSimpleNetworkStats
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 Delay;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float Ping;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 Rollback;
};

USTRUCT(BlueprintType)
struct FGekkoFullNetworkStats
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float KbSent = 0.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float KbReceived = 0.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 LastPing = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float AvgPing = 0.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float Jitter = 0.f;
};

USTRUCT(BlueprintType)
struct FGekkoDesyncInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 Frame = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 LocalChecksum = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 RemoteChecksum = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 RemoteHandle = 0;
};

USTRUCT(BlueprintType)
struct FGekkoConfig
{
	GENERATED_BODY()
	// The maximum amount of players present in the session.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 NumPlayers = 0;
	// The maximum amount of spectators present in the session.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 MaxSpectators = 0;
	// How many frames will be available for prediction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	uint8 InputPredictionWindow = 0;
	// The amount of delay between the playing clients and the spectator within frames.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SpectatorDelay = 0;
	// The size of the inputs stored within GekkoNet.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 InputSize = 0;
	// The size of states stored within GekkoNet.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StateSize = 0;
	// A reduced saving implementation for games that utilise very large save states. When rollback does occur, the game will advance more states that normal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLimitedSaving = false;
	// Checks whether a desync occurs, if your gameplay loop is deterministic, this does not need to be enabled.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDesyncDetection = false;
	// Used within stress test scenarios.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CheckDistance = 0;
};