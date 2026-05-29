// Fill out your copyright notice in the Description page of Project Settings.

#include "GekkoNetSubsystem.h"
#include "GekkoNetLocalAdapter.h"
#include "GekkoNetLog.h"
#include "GekkoNetSimulationInterface.h"
#include "GekkoNetSteamAdapter.h"
#include "GekkoNetUnrealAdapter.h"

#define STATS_UPDATE_TIMER_MAX 60
#define FRAME_SKIP_TIMER_MAX 60

int32 UGekkoNetSubsystem::AddActor(EGekkoPlayerType PlayerType, FString Address)
{
    if (Session == nullptr)
        return -1;
    
    auto Type = static_cast<GekkoPlayerType>(PlayerType);
    
    int32 ActorID;
    if (Address.IsEmpty())
    {
        ActorID = gekko_add_actor(Session, Type, nullptr);
        LocalPlayerIDs.Add(ActorID);
    }
    else
    {
        FTCHARToUTF8 Convert(*Address);
        GekkoNetAddress Remote
        {
            (void*)Convert.Get(),
            (unsigned int)Convert.Length()
        };
        ActorID = gekko_add_actor(Session, Type, &Remote);
#if WITH_EDITOR
        if (IsPlayInEditor())
        {
            GekkoNetLocalAdapter::MapLocalAddress(Address, ActorID);
        }
#endif
    }
    return ActorID;
}

void UGekkoNetSubsystem::StartSession(FGekkoConfig InConfig, bool IsSpectator)
{
    
    FMemory::Memzero(&Config, sizeof(Config));
    Config.num_players = InConfig.NumPlayers;
    Config.check_distance = InConfig.CheckDistance;
    Config.desync_detection = InConfig.bDesyncDetection;
    Config.input_prediction_window = InConfig.InputPredictionWindow;
    Config.input_size = InConfig.InputSize;
    Config.limited_saving =  InConfig.bLimitedSaving;
    Config.max_spectators = InConfig.MaxSpectators;
    Config.spectator_delay = InConfig.SpectatorDelay;
    Config.state_size = InConfig.StateSize;
    
    if (gekko_create    (&Session, IsSpectator ? GekkoSpectateSession : GekkoGameSession)) {
        gekko_start(Session, &Config);
    } else
    {
        UE_LOG(LogGekkoNet, Error, TEXT("Session is already running, failed to start a new one."));
        return;
    }
    
    LocalInputBuffer.SetNumZeroed(Config.input_size);
#if WITH_EDITOR
    if (IsPlayInEditor())
    {
        gekko_net_adapter_set(Session, GekkoNetLocalAdapter::GetLocalAdapter(LocalAdapterID));
        UE_LOG(LogGekkoNet, Log, TEXT("Started a local PIE session for player %d"), LocalAdapterID);
    }
    else
#endif
    {
        switch (TransportType)
        {
        case EGekkoTransportType::Asio:
            gekko_net_adapter_set(Session, gekko_default_adapter(LocalPort));
            break;
        case EGekkoTransportType::Unreal:
            gekko_net_adapter_set(Session, FGekkoNetAdapter::UE_Gekko_Adapter(LocalPort));
            break;
        case EGekkoTransportType::Steam:
            gekko_net_adapter_set(Session, GekkoNetSteamAdapter::Steam_Gekko_Adapter());
            break;
        }
        const UEnum* Enum = StaticEnum<EGekkoTransportType>();
        FString Name = Enum->GetNameStringByValue((int64)TransportType);
        if (TransportType != EGekkoTransportType::Steam)
        {
            UE_LOG(LogGekkoNet, Log, TEXT("Started a session at port %hu using %s sockets."), LocalPort, *Name);
        }
        else
        {
            UE_LOG(LogGekkoNet, Log, TEXT("Started a session using %s sockets."), *Name);
        }
    }
    SessionState = EGekkoSessionState::Running;
}

void UGekkoNetSubsystem::EndSession()
{
    if (Session == nullptr)
        return;
    
    gekko_destroy(&Session);
#if WITH_EDITOR
    if (IsPlayInEditor())
    {
        GekkoNetLocalAdapter::EmptyAddresses();
    }
    else
#endif
    {
        switch (TransportType)
        {
        case EGekkoTransportType::Asio:
            gekko_default_adapter_destroy();
            break;
        case EGekkoTransportType::Unreal:
            FGekkoNetAdapter::UE_Gekko_Adapter_Destroy();
            break;
        case EGekkoTransportType::Steam:
            break;
        }
    }
    
    SessionState = EGekkoSessionState::Inactive;
    
    LocalPlayerIDs.Empty();
    UE_LOG(LogGekkoNet, Warning, TEXT("Closing session."));
}

void UGekkoNetSubsystem::UpdateSession()
{
    if (!IsSessionRunning())
        return;
    
    RunSession();
}

bool UGekkoNetSubsystem::IsSessionRunning() const
{
    if (Session == nullptr)
        return false;
    
    return SessionState == EGekkoSessionState::Running;
}

void UGekkoNetSubsystem::RunSession()
{
    const bool CatchUp = NeedToCatchUp() && FrameSkipTimer == 0;
    StepLogic();
    
    if (CatchUp) 
    {
        const int32 FramesToSkip = FMath::Min(FramesAllowedToSkip, FramesBehind);
        for (int i = 0; i < FramesToSkip; ++i)
        {
            StepLogic(); 
        }
        FrameSkipTimer = FrameSkipTimerMax;
    }

    FrameSkipTimer -= 1;
    FrameSkipTimer = FMath::Max(FrameSkipTimer, 0);
}

#if WITH_EDITOR
bool UGekkoNetSubsystem::IsPlayInEditor() const
{
    if (!GEditor)
        return false;
    
    return GEditor->PlayWorld && !bDisablePlayInEditorAdapters;
}
#endif


void UGekkoNetSubsystem::HandleDisconnection(GekkoSessionEvent* Ev)
{
    FramesBehind = -gekko_frames_ahead(Session);
    OnPlayerDisconnected.Broadcast(Ev->data.disconnected.handle);
    
    EndSession();
}

void UGekkoNetSubsystem::StepLogic()
{
    ProcessSession();
    ProcessEvents();
}

void UGekkoNetSubsystem::AddLocalInputs()
{
    auto InputPtr = LocalInputBuffer.GetData();
    for (int i = 0; i < LocalPlayerIDs.Num(); ++i)
    {
        int32 LocalPlayer = LocalPlayerIDs[i];
        SimHost->GekkoGetLocalInput(LocalPlayer, InputPtr);
        
        gekko_add_local_input(Session, LocalPlayer, InputPtr);
    }
}

void UGekkoNetSubsystem::ProcessSession()
{
    if (!IsSessionRunning())
        return;
    
    FramesBehind = -gekko_frames_ahead(Session);
    gekko_network_poll(Session);
    
    AddLocalInputs();

    int SessionEventCount = 0;
    GekkoSessionEvent** session_events = gekko_session_events(Session, &SessionEventCount);
    for (int i = 0; i < SessionEventCount; ++i)
    {
        GekkoSessionEvent* Ev = session_events[i];
        switch (Ev->type)
        {
        case GekkoPlayerSyncing:
            {
                const int SyncHandle = Ev->data.syncing.handle;
                UE_LOG(LogGekkoNet, Verbose, TEXT("Player %d is syncing."), SyncHandle);
                OnPlayerSyncing.Broadcast(Ev->data.syncing.handle, Ev->data.syncing.current, Ev->data.syncing.max);
                break;
            }
        case GekkoPlayerConnected:
            {
                const int ConnectedHandle = Ev->data.connected.handle;
                UE_LOG(LogGekkoNet, Log, TEXT("Player %d has connected."), ConnectedHandle);
                OnPlayerConnected.Broadcast(Ev->data.connected.handle);
                break;
            }
        case GekkoPlayerDisconnected:
            {
                const int DisconnectedHandle = Ev->data.disconnected.handle;
                UE_LOG(LogGekkoNet, Warning, TEXT("Player %d has disconnected."), DisconnectedHandle);
                HandleDisconnection(Ev);
                break;
            }
        case GekkoSessionStarted:
            {
                OnSessionStarted.Broadcast();
                UE_LOG(LogGekkoNet, Log, TEXT("Session started."));
                break;
            }
        case GekkoSpectatorPaused:
            OnSpectatorPaused.Broadcast(Ev->data.connected.handle);
            break;
        case GekkoSpectatorUnpaused:
            OnSpectatorUnpaused.Broadcast(Ev->data.connected.handle);
            break;
        case GekkoDesyncDetected:
            {
                FGekkoDesyncInfo Info;
                Info.Frame = Ev->data.desynced.frame;
                Info.LocalChecksum = Ev->data.desynced.local_checksum;
                Info.RemoteChecksum = Ev->data.desynced.remote_checksum;
                Info.RemoteHandle = Ev->data.desynced.remote_handle;
                UE_LOG(LogGekkoNet, Warning, TEXT("Desync detected at frame %d"), Info.Frame);
                OnDesyncDetected.Broadcast(Info);
                break;
            }
        default:
            break;
        }
    }
}

void UGekkoNetSubsystem::ProcessEvents()
{
    if (!IsSessionRunning())
        return;
    
    int EventCount = 0;
    int FramesRolledBack = 0;
    GekkoGameEvent** Updates = gekko_update_session(Session, &EventCount);
    for (int i = 0; i < EventCount; ++i)
    {
        GekkoGameEvent* Ev = Updates[i];
        switch (Ev->type)
        {
        case GekkoSaveEvent:
            {
                SimHost->GekkoSave(Ev);
                UE_LOG(LogGekkoNet, VeryVerbose, TEXT("Gekko save called! Frame:%d Checksum:0x%08X"), Ev->data.save.frame, *Ev->data.save.checksum);
                break;
            }
        case GekkoLoadEvent:
            {
                SimHost->GekkoLoad(Ev);
                UE_LOG(LogGekkoNet, VeryVerbose, TEXT("Gekko load called! Frame:%d"), Ev->data.load.frame);
                break;
            }
        case GekkoAdvanceEvent:
            {
                const bool rolling_back = Ev->data.adv.rolling_back;
                SimHost->GekkoAdvance(Ev);
                FramesRolledBack += rolling_back ? 1 : 0;
                UE_LOG(LogGekkoNet, VeryVerbose, TEXT("Gekko Advance called Frame:%d"), Ev->data.adv.frame);
                break;
            }
        default:
            break;
        }
    }
    FrameMaxRollback = FMath::Max(FrameMaxRollback, FramesRolledBack);
}

FGekkoSimpleNetworkStats UGekkoNetSubsystem::UpdateNetworkStats(int32 Player)
{
    GekkoNetworkStats GNetStats;
    gekko_network_stats(Session, Player, &GNetStats);

    NetStats.Ping = GNetStats.avg_ping;
    NetStats.Delay = LocalDelay;

    if (FrameMaxRollback < NetStats.Rollback) {
        NetStats.Rollback -= 1;
    } else {
        NetStats.Rollback = FrameMaxRollback;
    }
    
    FrameMaxRollback = 0;
    return NetStats;
}

FGekkoFullNetworkStats UGekkoNetSubsystem::GetFullNetworkStats(int32 Player) const
{
    GekkoNetworkStats GNetStats;
    gekko_network_stats(Session, Player, &GNetStats);
    
    FGekkoFullNetworkStats FullNetStats;
    FullNetStats.AvgPing = GNetStats.avg_ping;
    FullNetStats.Jitter = GNetStats.jitter;
    FullNetStats.KbReceived = GNetStats.kb_received;
    FullNetStats.KbSent = GNetStats.kb_sent;
    
    return FullNetStats;
}

void UGekkoNetSubsystem::SetLocalPort(int32 NewLocalPort)
{
    LocalPort = NewLocalPort;
}

#if WITH_EDITOR
void UGekkoNetSubsystem::SetLocalAdapter(int32 Index)
{
    LocalAdapterID = Index;
}
#endif

void UGekkoNetSubsystem::SetTransportType(EGekkoTransportType Type)
{
    TransportType = Type;
}

bool UGekkoNetSubsystem::SetSimulationHost(TScriptInterface<IGekkoNetSimulationInterface> NewHost)
{
    SimHost = NewHost;
    return SimHost.GetObject() ? true : false;
}

bool UGekkoNetSubsystem::SetLocalDelay(int32 Delay, int32 LocalPlayer)
{
    if (Session == nullptr)
        return false;

    if (LocalPlayer < 0 || LocalPlayer > Config.num_players)
        return false;
    
    LocalDelay = FMath::Max(Delay, 0);
    gekko_set_local_delay(Session, LocalPlayer, LocalDelay);
    UE_LOG(LogGekkoNet, Log, TEXT("Gekko Delay has been updated to %d for Player %d."), LocalDelay, LocalPlayer);
    
    return true;
}

bool UGekkoNetSubsystem::SetRunahead(int32 Runahead)
{
    if (Session == nullptr)
        return false;
    
    LocalRunahead = FMath::Max(Runahead, 0);
    gekko_set_runahead(Session, LocalRunahead);
    UE_LOG(LogGekkoNet, Log, TEXT("Gekko Runahead has been updated to %d."), LocalRunahead);
    
    return true;
}

bool UGekkoNetSubsystem::NeedToCatchUp() const
{
    return FramesBehind >= 1;
}
