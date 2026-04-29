// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GekkoNetTypes.generated.h"

#define DEFAULT_INPUT_DELAY 1

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
	
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float KbSent = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float KbReceived = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") int32 LastPing = 0;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float AvgPing = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "GekkoNet") float Jitter = 0.f;
	
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
struct FGekkoSessionSize
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InputSize = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StateSize = 0;
};

USTRUCT(BlueprintType)
struct FGekkoSessionConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGekkoSessionType SessionType = EGekkoSessionType::Game;
	
	// Indexes of the Players.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGekkoPlayerPeer> Players;
	
	// The connectivity method of the session.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGekkoTransportType Transport = EGekkoTransportType::LocalOnly;
	
	// The data size required to store the input and state.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGekkoSessionSize SessionSize;
	
	// How many frames ahead the game state will run.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	int32 FramesRunahead = 8;
	
	// Whether or not data saving is kept to a minimum.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet")
	bool LimitedSaving = false;
	
	// Detects whether a desync has occured. Should be primarily used only in debugging situations Can only be enabled if Limited Saving is disabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet", meta=(EditCondition="!LimitedSaving"))
	bool DesyncDetection = false;
	
	// How many states that are stored, if Limited saving is enabled, this value is ignored.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet", meta=(EditCondition="!LimitedSaving"))
	int32 InputPredictionWindow = 10;
	
	// The amount of spectators allowed for the current session.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet|Spectator")
	int32 MaxSpectators = 0;
	
	// The amount of delay between the match and the spectators.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet|Spectator", meta=(EditCondition="MaxSpectators>0"))
	int32 SpectatorDelay = 300;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GekkoNet|Stress")
	int32 CheckDistance = 0;
	
	void AddPlayer(FString Address = "", int32 Port = 7000, bool IsSpectating)
	{
		FGekkoPlayerPeer Peer {};
		if (IsSpectating)
		{
			Peer.PlayerType = EGekkoPlayerType::Spectator;
		}
		else
		{
			Peer.PlayerType = Address.IsEmpty() ? EGekkoPlayerType::LocalPlayer : EGekkoPlayerType::RemotePlayer;
		}

		if (Peer.PlayerType == EGekkoPlayerType::RemotePlayer)
		{
			Peer.RemoteInfo.Address = Address;
			Peer.RemoteInfo.RemotePort = Port;
		}
		
		Players.Add(Peer);
	}
};