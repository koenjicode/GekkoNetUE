// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "gekkonet.h"
#include "UObject/Interface.h"
#include "GekkoNetSimulationInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGekkoNetSimulationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GEKKONETUE_API IGekkoNetSimulationInterface
{
	GENERATED_BODY()

public:
	
	// Grab local inputs.
	virtual void GekkoGetLocalInputs(void* OutInputData);
	
	// Handle saving.
	virtual void GekkoSave(GekkoGameEvent* Event);
	// Handle loading.
	virtual void GekkoLoad(GekkoGameEvent* Event);

	// Advance the game state with a choice to render the visual output.
	virtual void GekkoAdvance(GekkoGameEvent* Event, bool Render);
	// Handle disconnecting the game when a player has left.
	virtual void GekkoDisconnect(GekkoSessionEvent* Event);
};
