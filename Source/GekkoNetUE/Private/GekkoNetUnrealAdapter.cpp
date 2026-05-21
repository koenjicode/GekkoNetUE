#include "GekkoNetUnrealAdapter.h"

#include "GekkoNetLog.h"
#include "Common/UdpSocketBuilder.h"

static FSocket* UnrealSocket = nullptr;
static uint8 Buffer[1024];
static TArray<GekkoNetResult*> Results;

static void UE_SendData(GekkoNetAddress* Addr, const char* Data, int Length)
{
    FString AddressStr(Addr->size, (char*)Addr->data);
    if (AddressStr.IsEmpty())
    {
        return;
    }
    
    TSharedRef<FInternetAddr> RemoteAddr = ISocketSubsystem::Get()->CreateInternetAddr();
    bool bIsValid = false;
    
    RemoteAddr->SetIp(*AddressStr, bIsValid);
    if (!bIsValid)
    {
        return;
    }
    
    int32 BytesSent = 0;
    UnrealSocket->SendTo((uint8*)Data, Length, BytesSent, *RemoteAddr);
}

static GekkoNetResult** UE_ReceiveData(int* OutLength)
{
    Results.Reset();

    uint32 PendingSize = 0;
    while (UnrealSocket->HasPendingData(PendingSize))
    {
        TSharedRef<FInternetAddr> SenderAddr = ISocketSubsystem::Get()->CreateInternetAddr();
        int32 BytesRead = 0;
        const bool bSuccess = UnrealSocket->RecvFrom(Buffer, sizeof(Buffer), BytesRead, *SenderAddr);
        if (!bSuccess || BytesRead <= 0)
        {
            continue;
        }
        GekkoNetResult* Res = reinterpret_cast<GekkoNetResult*>(FMemory::Malloc(sizeof(GekkoNetResult)));
        
        Res->data_len = BytesRead;
        Res->data = FMemory::Malloc(BytesRead);
        FMemory::Memcpy(Res->data, Buffer, BytesRead);
        
        FString AddrStr = SenderAddr->ToString(true);
        
        FTCHARToUTF8 AddrUtf8(*AddrStr);
        char* AddrData = (char*)FMemory::Malloc(AddrUtf8.Length());
        FMemory::Memcpy(AddrData,AddrUtf8.Get(),AddrUtf8.Length());
        Res->addr.data = AddrData;
        Res->addr.size = AddrUtf8.Length();

        Results.Add(Res);
    }
    
    *OutLength = Results.Num();
    return Results.GetData();
}

static void UE_FreeData(void* DataPtr)
{
    FMemory::Free(DataPtr);
}

static GekkoNetAdapter UE_DefaultSocket{
    UE_SendData,
    UE_ReceiveData,
    UE_FreeData
};

GekkoNetAdapter* FGekkoNetAdapter::UE_Gekko_Adapter(int32 Port)
{
    if (UnrealSocket)
    {
        UE_Gekko_Adapter_Destroy();
    }

    UnrealSocket = FUdpSocketBuilder(TEXT("UnrealGekko"))
    .AsNonBlocking()
    .AsReusable()
    .BoundToPort(Port)
    .Build();
    
    if (!UnrealSocket)
    {
        UE_LOG(LogGekkoNet, Error, TEXT("Unreal Socket was unable to be created for GekkoNet for port %d!"), Port);
        return nullptr;
    }
    return &UE_DefaultSocket;
}

void FGekkoNetAdapter::UE_Gekko_Adapter_Destroy()
{
    if (UnrealSocket)
    {
        UnrealSocket->Close();
        ISocketSubsystem::Get()->DestroySocket(UnrealSocket);
        UnrealSocket = nullptr;
    }
}