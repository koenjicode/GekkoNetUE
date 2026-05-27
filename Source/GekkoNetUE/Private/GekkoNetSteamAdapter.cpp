#include "GekkoNetSteamAdapter.h"

static void Steam_Send(GekkoNetAddress* Addr, const char* Data, int Length) 
{
	FString AddressStr(Addr->size, (char*)Addr->data);
	if (AddressStr.IsEmpty())
	{
		return;
	}
}

static GekkoNetResult** Steam_Receive(int* Length) 
{
	return new GekkoNetResult*;
}

static void Steam_Free(void* data_ptr) 
{
	FMemory::Free(data_ptr);
}

static GekkoNetAdapter SteamGekkoSocket
{
	Steam_Send,
	Steam_Receive,
	Steam_Free
};

GekkoNetAdapter* GekkoNetSteamAdapter::Steam_Gekko_Adapter()
{
	return &SteamGekkoSocket;
}

void GekkoNetSteamAdapter::Steam_Gekko_Adapter_Destroy()
{
}
