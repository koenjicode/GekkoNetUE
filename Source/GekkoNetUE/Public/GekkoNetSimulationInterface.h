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
	
	virtual void GekkoGetLocalInputs(void* OutInputData) = 0;
	virtual void GekkoSave(GekkoGameEvent* Event) = 0;
	virtual void GekkoLoad(GekkoGameEvent* Event) = 0;
	virtual void GekkoAdvance(GekkoGameEvent* Event, bool Render) = 0;
	virtual void GekkoDisconnect(GekkoSessionEvent* Event) = 0;
};
