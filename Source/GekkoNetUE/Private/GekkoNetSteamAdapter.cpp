#include "GekkoNetSteamAdapter.h"
#include "SocketSubsystem.h"
#include "GekkoNetLog.h"
#include "Sockets.h"

static ISocketSubsystem* SteamSubsystem = nullptr;
static uint8 SteamBuffer[1024];
static TArray<GekkoNetResult*> SteamResults;
static FSocket* SteamSocket = nullptr;

static void Steam_Send(GekkoNetAddress* Addr, const char* Data, int Length) 
{
	FString SteamStr(Addr->size, (char*)Addr->data);
	if (SteamStr.IsEmpty())
	{
		return;
	}
	
	TSharedRef<FInternetAddr> RemoteAddr = ISocketSubsystem::Get()->CreateInternetAddr();
	bool bIsValid = false;
    
	RemoteAddr->SetIp(*SteamStr, bIsValid);
	if (!bIsValid)
	{
		return;
	}
    
	int32 BytesSent = 0;
	SteamSocket->SendTo((uint8*)Data, Length, BytesSent, *RemoteAddr);
}

static GekkoNetResult** Steam_Receive(int* Length) 
{
	SteamResults.Reset();

	uint32 PendingSize = 0;
	while (SteamSocket->HasPendingData(PendingSize))
	{
		TSharedRef<FInternetAddr> SenderAddr = ISocketSubsystem::Get()->CreateInternetAddr();
		int32 BytesRead = 0;
		const bool bSuccess = SteamSocket->RecvFrom(SteamBuffer, sizeof(SteamBuffer), BytesRead, *SenderAddr);
		if (!bSuccess || BytesRead <= 0)
		{
			continue;
		}
		GekkoNetResult* Res = reinterpret_cast<GekkoNetResult*>(FMemory::Malloc(sizeof(GekkoNetResult)));
        
		Res->data_len = BytesRead;
		Res->data = FMemory::Malloc(BytesRead);
		FMemory::Memcpy(Res->data, SteamBuffer, BytesRead);
        
		FString AddrStr = SenderAddr->ToString(true);
        
		FTCHARToUTF8 AddrUtf8(*AddrStr);
		char* AddrData = (char*)FMemory::Malloc(AddrUtf8.Length());
		FMemory::Memcpy(AddrData,AddrUtf8.Get(),AddrUtf8.Length());
		Res->addr.data = AddrData;
		Res->addr.size = AddrUtf8.Length();

		SteamResults.Add(Res);
	}
    
	*Length = SteamResults.Num();
	return SteamResults.GetData();
}

static void Steam_Free(void* data_ptr) 
{
	FMemory::Free(data_ptr);
}

static GekkoNetAdapter SteamAdapter
{
	Steam_Send,
	Steam_Receive,
	Steam_Free
};

GekkoNetAdapter* GekkoNetSteamAdapter::Steam_Gekko_Adapter()
{
	SteamSubsystem = ISocketSubsystem::Get(FName("SteamSockets"));
	
	if (!SteamSubsystem)
	{
		UE_LOG(LogGekkoNet, Error, TEXT("Steam socket subsystem unavailable!"));
		return nullptr;
	}
	
	SteamSocket = SteamSubsystem->CreateSocket(NAME_DGram,TEXT("GekkoSteamSocket"),false);
	
	if (!SteamSocket)
	{
		UE_LOG(LogGekkoNet, Error, TEXT("Failed to create Steam socket!"));
		return nullptr;
	}
	
	TSharedRef<FInternetAddr> BindAddr = SteamSubsystem->CreateInternetAddr();

	BindAddr->SetAnyAddress();
	BindAddr->SetPort(0);

	if (!SteamSocket->Bind(*BindAddr))
	{
		UE_LOG(LogGekkoNet, Error, TEXT("Failed to bind Steam socket"));

		SteamSubsystem->DestroySocket(SteamSocket);
		SteamSocket = nullptr;

		return nullptr;
	}

	SteamSocket->SetNonBlocking(true);

	return &SteamAdapter;
}

void GekkoNetSteamAdapter::Steam_Gekko_Adapter_Destroy()
{
	if (!SteamSocket)
	{
		return;
	}
	
	SteamSocket->Close();
	ISocketSubsystem::Get()->DestroySocket(SteamSocket);
	SteamSocket = nullptr;
}
