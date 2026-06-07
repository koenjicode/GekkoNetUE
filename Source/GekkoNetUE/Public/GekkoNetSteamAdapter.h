#pragma once
#include "gekkonet.h"

class FGekkoNetSteamAdapter
{
public:
	static GekkoNetAdapter* Steam_Gekko_Adapter();
	static void Steam_Gekko_Adapter_Destroy();
};
