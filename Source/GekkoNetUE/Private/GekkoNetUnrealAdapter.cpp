#include "GekkoNetUnrealAdapter.h"
#include "GekkoNetLog.h"
#include "Common/UdpSocketBuilder.h"

static FSocket* GekkoSocket = nullptr;
static uint8 GekkoReceiveBuffer[1024];
static TArray<GekkoNetResult*> Results;

static void UE_SendData(GekkoNetAddress* Addr, const char* Data, int Length)
{
    FString NetStr(Addr->size, (char*)Addr->data);
    if (NetStr.IsEmpty())
    {
        return;
    }
    
    FString AddrStr;
    FString PortStr;
    
    NetStr.Split(TEXT(":"), &AddrStr, &PortStr);
    
    TSharedRef<FInternetAddr> RemoteAddr = ISocketSubsystem::Get()->CreateInternetAddr();
    bool bIsValid = false;
    
    RemoteAddr->SetIp(*AddrStr, bIsValid);
    if (!bIsValid)
    {
        return;
    }
    const int32 RemotePort = FCString::Atoi(*PortStr);
    RemoteAddr->SetPort(RemotePort);
    
    int32 BytesSent = 0;
    GekkoSocket->SendTo((uint8*)Data, Length, BytesSent, *RemoteAddr);
}

static GekkoNetResult** UE_ReceiveData(int* OutLength)
{
    Results.Reset();

    uint32 PendingSize = 0;
    while (GekkoSocket->HasPendingData(PendingSize))
    {
        TSharedRef<FInternetAddr> SenderAddr = ISocketSubsystem::Get()->CreateInternetAddr();
        int32 BytesRead = 0;
        const bool bSuccess = GekkoSocket->RecvFrom(GekkoReceiveBuffer, sizeof(GekkoReceiveBuffer), BytesRead, *SenderAddr);
        if (!bSuccess || BytesRead <= 0)
        {
            continue;
        }
        GekkoNetResult* Res = reinterpret_cast<GekkoNetResult*>(FMemory::Malloc(sizeof(GekkoNetResult)));
        
        Res->data_len = BytesRead;
        Res->data = FMemory::Malloc(BytesRead);
        FMemory::Memcpy(Res->data, GekkoReceiveBuffer, BytesRead);
        
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

GekkoNetAdapter* FGekkoNetAdapter::UE_Gekko_Adapter(FIPv4Endpoint Endpoint)
{
    if (GekkoSocket)
    {
        UE_Gekko_Adapter_Destroy();
    }

    GekkoSocket = FUdpSocketBuilder(TEXT("GekkoNet Unreal Socket"))
    .AsNonBlocking()
    .BoundToEndpoint(Endpoint)
    .WithReceiveBufferSize(sizeof(GekkoReceiveBuffer))
    .Build();
    
    if (!GekkoSocket)
    {
        return nullptr;
    }
    
    UE_LOG(LogGekkoNet, Log, TEXT("GekkoNet Unreal Socket created with addr %s"), *Endpoint.ToString());
    return &UE_DefaultSocket;
}

void FGekkoNetAdapter::UE_Gekko_Adapter_Destroy()
{
    if (GekkoSocket)
    {
        GekkoSocket->Close();
        
        ISocketSubsystem::Get()->DestroySocket(GekkoSocket);
        GekkoSocket = nullptr;
    }
}