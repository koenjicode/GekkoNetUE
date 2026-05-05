// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GekkoNetTypes.generated.h"

#define DEFAULT_INPUT_DELAY 1
#define FRAME_MAX_ROLLBACK 9

UENUM()
enum class EGekkoSessionState : uint8
{
	Idling			UMETA(DisplayName = "Session Idle"),
	Transitioning	UMETA(DisplayName = "Session Transitioning"),
	Connecting		UMETA(DisplayName = "Session Connecting"),
	Running			UMETA(DisplayName = "Session Running"),
	Exiting			UMETA(DisplayName = "Session Exiting"),
};

UENUM(BlueprintType)
enum class EGekkoTransportType : uint8
{
	LocalOnly,
	RawIP,
	EOS,
	Steam,
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
struct FGekkoNetworkStats
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
	
	// out stats
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float KbSent = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float KbReceived = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 LastPing = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float Ping = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float Jitter = 0.f;
	
	// calculated
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 Delay = DEFAULT_INPUT_DELAY;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 Rollback = FRAME_MAX_ROLLBACK;
	
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

USTRUCT(BlueprintType)
struct FGekkoRemoteData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Address;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RemotePort = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlatformUserId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SocketId;
};

USTRUCT(BlueprintType)
struct FGekkoPlayerPeer
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGekkoPlayerType PlayerType = EGekkoPlayerType::LocalPlayer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Local", meta=(EditCondition="PlayerType == EGekkoPlayerType::LocalPlayer", EditConditionHides))
	int32 LocalInputDelay = DEFAULT_INPUT_DELAY;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Remote", meta=(EditCondition="PlayerType == EGekkoPlayerType::RemotePlayer", EditConditionHides))
	FGekkoRemoteData RemoteInfo;
};

USTRUCT(BlueprintType)
struct FGekkoSessionConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	int32 NumPlayers = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	int32 MaxSpectators = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	int32 InputPredictionWindow = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet|Spectator")
	int32 SpectatorDelay = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	int32 InputSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	int32 StateSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	bool bLimitedSaving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet|Stress")
	int32 CheckDistance = 0;
};