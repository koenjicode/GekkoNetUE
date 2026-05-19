#pragma once
#include "gekkonet.h"

struct FPlayerAddressInfo
{
	FString Address;
	int32 Index;
};

class GekkoNetLocalAdapter
{
public:
	static void EmptyAddresses();
	static GekkoNetAdapter* GetLocalAdapter(int32 InIndex = -1);
	
	static void MapLocalAddress(FString Address, uint8 LocalPlayer);
};
