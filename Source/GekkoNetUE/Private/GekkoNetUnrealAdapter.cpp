// Fill out your copyright notice in the Description page of Project Settings.


#include "GekkoNetUnrealAdapter.h"
#include "Common/UdpSocketBuilder.h"

static FSocket* GekkoSocket = nullptr;

static void UE_SendData(GekkoNetAddress* Addr, const char* Data, int Length)
{
    // Addr->data is the address string e.g. "127.0.0.1:7000"
    // Parse it and send via UE socket
    FString AddressStr(Addr->size, (char*)Addr->data);
    
    ISocketSubsystem* SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    TSharedRef<FInternetAddr> Dest = SocketSub->CreateInternetAddr();
    bool bValid;
    Dest->SetIp(*AddressStr, bValid);
    
    int32 BytesSent = 0;
    GekkoSocket->SendTo((uint8*)Data, Length, BytesSent, *Dest);
}

static GekkoNetResult** UE_ReceiveData(int* OutLength)
{
    // Poll socket for all pending packets
    static TArray<GekkoNetResult*> Results;
    Results.Reset();

    uint32 PendingSize = 0;
    while (GekkoSocket->HasPendingData(PendingSize))
    {
        GekkoNetResult* Result = (GekkoNetResult*)FMemory::Malloc(sizeof(GekkoNetResult));
        Result->data = FMemory::Malloc(PendingSize);
        Result->data_len = PendingSize;

        TSharedRef<FInternetAddr> SenderAddr = 
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
        int32 BytesRead = 0;
        GekkoSocket->RecvFrom((uint8*)Result->data, PendingSize, BytesRead, *SenderAddr);
        Result->data_len = BytesRead;

        FString AddrStr = SenderAddr->ToString(true);
        char* AddrData = (char*)FMemory::Malloc(AddrStr.Len());
        FMemory::Memcpy(AddrData, TCHAR_TO_ANSI(*AddrStr), AddrStr.Len());
        Result->addr.data = AddrData;
        Result->addr.size = AddrStr.Len();

        Results.Add(Result);
    }

    *OutLength = Results.Num();
    return Results.Num() > 0 ? Results.GetData() : nullptr;
}

static void UE_FreeData(void* DataPtr)
{
    FMemory::Free(DataPtr);
}

static GekkoNetAdapter UEGekkoAdapter{
    UE_SendData,
    UE_ReceiveData,
    UE_FreeData
};

GekkoNetAdapter* FGekkoNetAdapter::UE_Gekko_Adapter(int32 Port)
{
    if (GekkoSocket)
    {
        UE_Gekko_Adapter_Destroy();
    }

    GekkoSocket = FUdpSocketBuilder(TEXT("GekkoNetSocket"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(Port)
        .Build();

    if (!GekkoSocket)
    {
        return nullptr;
    }

    return &UEGekkoAdapter;
}

void FGekkoNetAdapter::UE_Gekko_Adapter_Destroy()
{
    if (GekkoSocket)
    {
        GekkoSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(GekkoSocket);
        GekkoSocket = nullptr;
    }
}