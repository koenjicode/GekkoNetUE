#include "GekkoNetLocalAdapter.h"

static TArray<GekkoNetResult*> ResultsP1;
static TQueue<TArray<uint8>> InPacketsP1;

static TArray<GekkoNetResult*> ResultsP2;
static TQueue<TArray<uint8>> InPacketsP2;

static void UE_LocalSendP1(GekkoNetAddress* Addr, const char* Data, int Length)
{
    TArray<uint8> Packet;
    Packet.Append(reinterpret_cast<const uint8*>(Data), Length);
    InPacketsP2.Enqueue(Packet);
}

static void UE_LocalSendP2(GekkoNetAddress* Addr, const char* Data, int Length)
{
    TArray<uint8> Packet;
    Packet.Append(reinterpret_cast<const uint8*>(Data), Length);
    InPacketsP1.Enqueue(Packet);
}

static GekkoNetResult** ReceivePackets(TQueue<TArray<uint8>>& InPackets, TArray<GekkoNetResult*>& Results, const TCHAR* FakeAddress, int* OutLength)
{
    Results.Reset();

    TArray<uint8> Packet;

    while (InPackets.Dequeue(Packet))
    {
        GekkoNetResult* Result = static_cast<GekkoNetResult*>(FMemory::Malloc(sizeof(GekkoNetResult)));

        FMemory::Memzero( Result,sizeof(GekkoNetResult));

        Result->data_len = Packet.Num();
        Result->data = FMemory::Malloc(Packet.Num());
        FMemory::Memcpy(Result->data,Packet.GetData(), Packet.Num());

        FTCHARToUTF8 Convert(FakeAddress);
        Result->addr.size = Convert.Length();
        Result->addr.data = FMemory::Malloc(Result->addr.size);

        FMemory::Memcpy(Result->addr.data, Convert.Get(), Result->addr.size);
        Results.Add(Result);
    }

    *OutLength = Results.Num();
    return Results.Num() > 0 ? Results.GetData() : nullptr;
}

static GekkoNetResult** UE_LocalReceiveP1(int* OutLength)
{
    return ReceivePackets(InPacketsP1,ResultsP1,TEXT("Player2"),OutLength);
}

static GekkoNetResult** UE_LocalReceiveP2(int* OutLength)
{
    return ReceivePackets(InPacketsP2,ResultsP2,TEXT("Player1"), OutLength);
}

static void UE_LocalFree(void* DataPtr)
{
    if (DataPtr)
    {
        FMemory::Free(DataPtr);
    }
}

static GekkoNetAdapter UnrealLocalAdapterP1
{
    UE_LocalSendP1,
    UE_LocalReceiveP1,
    UE_LocalFree,
};

static GekkoNetAdapter UnrealLocalAdapterP2
{
    UE_LocalSendP2,
    UE_LocalReceiveP2,
    UE_LocalFree,
};

GekkoNetAdapter* GekkoNetLocalAdapter::GetLocalAdapter(int32 Index)
{
    switch (Index)
    {
    case 0:
        return &UnrealLocalAdapterP1;
    case 1:
        return &UnrealLocalAdapterP2;
    default:
        return nullptr;
    }
}