#if WITH_EDITOR

#include "GekkoNetLocalAdapter.h"

static TMap<FString, uint8> AddressMap;
static TMap<uint8, FString> ReverseAddressMap;

static TArray<GekkoNetResult*> ResultsP1;
static TQueue<TArray<uint8>> InPacketsP1;

static TArray<GekkoNetResult*> ResultsP2;
static TQueue<TArray<uint8>> InPacketsP2;

static TArray<GekkoNetResult*> ResultsP3;
static TQueue<TArray<uint8>> InPacketsP3;

static TArray<GekkoNetResult*> ResultsP4;
static TQueue<TArray<uint8>> InPacketsP4;

static uint8 NextAdapterIndex = 0;

static TQueue<TArray<uint8>>& GetInbox(int32 PlayerIndex)
{
    switch (PlayerIndex)
    {
    case 0: return InPacketsP1;
    case 1: return InPacketsP2;
    case 2: return InPacketsP3;
    case 3: return InPacketsP4;
    default: return InPacketsP1;
    }
}

static void UE_LocalSend(uint8 Sender, GekkoNetAddress* Addr, const char* Data, int Length)
{
    FString TargetStr = FString(UTF8_TO_TCHAR((const char*)Addr->data));
    auto Target = AddressMap.FindRef(TargetStr);

    TArray<uint8> Packet;
    Packet.Add(Sender); // sender identity
    Packet.Append((uint8*)Data, Length);
    
    // UE_LOG(LogTemp, Warning, TEXT("Sending: Sender=%d Target=%d Size=%d"), Sender, Target, Packet.Num());
    GetInbox(Target).Enqueue(Packet);
}

static GekkoNetResult** ReceivePackets(uint8 Receiver, TQueue<TArray<uint8>>& InPackets, TArray<GekkoNetResult*>& Results, int* OutLength)
{
    Results.Reset();
    TArray<uint8> Packet;

    while (InPackets.Dequeue(Packet))
    {
        uint8 Sender = Packet[0];

        GekkoNetResult* Result = static_cast<GekkoNetResult*>(FMemory::Malloc(sizeof(GekkoNetResult)));
        FMemory::Memzero(Result, sizeof(GekkoNetResult));

        Result->data_len = Packet.Num() - sizeof(uint8);
        Result->data = FMemory::Malloc(Result->data_len);
        FMemory::Memcpy(Result->data, Packet.GetData() + sizeof(uint8), Result->data_len);
        
        FString AddrStr = ReverseAddressMap.FindRef(Sender);
        FTCHARToUTF8 Convert(AddrStr);

        Result->addr.size = Convert.Length();
        Result->addr.data = FMemory::Malloc(Result->addr.size);

        FMemory::Memcpy(Result->addr.data, Convert.Get(), Result->addr.size);
        Results.Add(Result);
    }

    *OutLength = Results.Num();
    return Results.Num() > 0 ? Results.GetData() : nullptr;
}

static void UE_LocalSendP1(GekkoNetAddress* Addr, const char* Data, int Length)
{
    return UE_LocalSend(0, Addr, Data, Length);
}

static void UE_LocalSendP2(GekkoNetAddress* Addr, const char* Data, int Length)
{
    return UE_LocalSend(1, Addr, Data, Length);
}

static void UE_LocalSendP3(GekkoNetAddress* Addr, const char* Data, int Length)
{
    return UE_LocalSend(2, Addr, Data, Length);
}

static void UE_LocalSendP4(GekkoNetAddress* Addr, const char* Data, int Length)
{
    return UE_LocalSend(3, Addr, Data, Length);
}

static GekkoNetResult** UE_LocalReceiveP1(int* OutLength)
{
    return ReceivePackets(0, InPacketsP1, ResultsP1, OutLength);
}

static GekkoNetResult** UE_LocalReceiveP2(int* OutLength)
{
    return ReceivePackets(1, InPacketsP2, ResultsP2, OutLength);
}

static GekkoNetResult** UE_LocalReceiveP3(int* OutLength)
{
    return ReceivePackets(2, InPacketsP3, ResultsP3, OutLength);
}

static GekkoNetResult** UE_LocalReceiveP4(int* OutLength)
{
    return ReceivePackets(3, InPacketsP4, ResultsP4, OutLength);
}

static void UE_LocalFree(void* DataPtr)
{
    if (DataPtr)
    {
        FMemory::Free(DataPtr);
    }
}

static GekkoNetAdapter UnrealLocalAdapterP1{ UE_LocalSendP1, UE_LocalReceiveP1, UE_LocalFree };
static GekkoNetAdapter UnrealLocalAdapterP2{ UE_LocalSendP2, UE_LocalReceiveP2, UE_LocalFree };
static GekkoNetAdapter UnrealLocalAdapterP3{ UE_LocalSendP3, UE_LocalReceiveP3, UE_LocalFree };
static GekkoNetAdapter UnrealLocalAdapterP4{ UE_LocalSendP4, UE_LocalReceiveP4, UE_LocalFree };

void GekkoNetLocalAdapter::EmptyAddresses()
{
    NextAdapterIndex = 0;
    AddressMap.Empty();
}

GekkoNetAdapter* GekkoNetLocalAdapter::GetLocalAdapter(int32 InIndex)
{
    switch (InIndex)
    {
    case 0:
        return &UnrealLocalAdapterP1;
    case 1:
        return &UnrealLocalAdapterP2;
    case 2:
        return &UnrealLocalAdapterP3;
    case 3:
        return &UnrealLocalAdapterP4;
    default:
        return nullptr;
    }
}

void GekkoNetLocalAdapter::MapLocalAddress(FString Address, uint8 LocalPlayer)
{
    if (AddressMap.Contains(Address))
        return;
    
    AddressMap.Add(Address, LocalPlayer);
    ReverseAddressMap.Add(LocalPlayer, Address);
}

#endif

