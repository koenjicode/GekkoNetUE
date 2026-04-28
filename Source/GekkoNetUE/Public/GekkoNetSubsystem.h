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
	
	UFUNCTION(BlueprintCallable)
	bool Create(const FGekkoSessionConfig& Config, bool IsSpectator);
	UFUNCTION(BlueprintCallable)
	void Destroy();
	
	UFUNCTION(BlueprintCallable)
	bool SetLocalDelay(int32 Player, int32 Delay);
	UFUNCTION(BlueprintCallable)
	bool SetRunahead(int32 Runahead);
	
	UFUNCTION(BlueprintPure)
	bool IsSessionActive() const { return session != nullptr; }
	UFUNCTION(BlueprintPure)
	bool GetNetworkStats(int32 Player, FGekkoNetworkStats& OutStats) const;
	UFUNCTION(BlueprintPure)
	float GetFramesAhead() const;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGekkoSessionConfig CurrentSessionConfig;
    
private:
    
	GekkoSession* session;
};
