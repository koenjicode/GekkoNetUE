#pragma once
#if WITH_EDITOR
#include "gekkonet.h"

class GekkoNetLocalAdapter
{
public:
	static void EmptyAddresses();
	static GekkoNetAdapter* GetLocalAdapter(int32 InIndex = -1);
	
	static void MapLocalAddress(FString Address, uint8 LocalPlayer);
};
#endif
