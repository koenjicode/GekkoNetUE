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
	Game			UMETA(DisplayName = "Game Session"),
	Spectator		UMETA(DisplayName = "Spectator Session"),
	Stress			UMETA(DisplayName = "Stress Session"),
};

USTRUCT(BlueprintType)
struct FGekkoSimpleNetworkStats
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly) int Delay;
	UPROPERTY(BlueprintReadOnly) int Ping;
	UPROPERTY(BlueprintReadOnly) int Rollback;
};

USTRUCT(BlueprintType)
struct FGekkoFullNetworkStats
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float KbSent = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float KbReceived = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 LastPing = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float AvgPing = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float Jitter = 0.f;
};

USTRUCT(BlueprintType)
struct FGekkoDesyncInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 Frame = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 LocalChecksum = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 RemoteChecksum = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 RemoteHandle = 0;
};

USTRUCT()
struct FGekkoPeer
{
	GENERATED_BODY()
	
	UPROPERTY()
	FName Nickname;
};

USTRUCT(BlueprintType)
struct FGekkoConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 NumPlayers = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 MaxSpectators = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 InputPredictionWindow = DEFAULT_INPUT_DELAY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SpectatorDelay = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InputSize = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StateSize = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLimitedSaving = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDesyncDetection = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CheckDistance = 0;
};