// Fill out your copyright notice in the Description page of Project Settings.

#include "GekkoNetSubsystem.h"
#include "GekkoNetLocalAdapter.h"
#include "GekkoNetLog.h"
#include "GekkoNetSimulationInterface.h"

#define STATS_UPDATE_TIMER_MAX 60
#define FRAME_SKIP_TIMER_MAX 60

int32 UGekkoNetSubsystem::AddActor(EGekkoPlayerType PlayerType, FString Address)
{
    if (Session == nullptr)
        return -1;
    
    bool bPlayInEditor = GEditor && GEditor->PlayWorld;
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
        if (bPlayInEditor)
        {
            GekkoNetLocalAdapter::MapLocalAddress(Address, ActorID);
        }
    }
    return ActorID;
}

void UGekkoNetSubsystem::StartSession(FGekkoConfig InConfig, int32 InLocalPort, bool IsSpectator)
{
    if (InLocalPort == 0)
        return;
    
    bool bPlayInEditor = GEditor && GEditor->PlayWorld;
    
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
    
    if (gekko_create(&Session, IsSpectator ? GekkoSpectateSession : GekkoGameSession)) {
        gekko_start(Session, &Config);
    } else
    {
        UE_LOG(LogGekkoNet, Error, TEXT("Session is already running, failed to start a new one."));
        return;
    }
    
    LocalInputBuffer.SetNumZeroed(Config.input_size);
    
    if (bPlayInEditor)
    {
        gekko_net_adapter_set(Session, GekkoNetLocalAdapter::GetLocalAdapter(LocalAdapterID));
        UE_LOG(LogGekkoNet, Log, TEXT("Started a local PIE session for player %d"), LocalAdapterID);
    }
    else
    {
        gekko_net_adapter_set(Session, gekko_default_adapter(InLocalPort));
        UE_LOG(LogGekkoNet, Log, TEXT("Starting a session at port %hu\n"), InLocalPort);
    }
    SessionState = EGekkoSessionState::Running;
}

void UGekkoNetSubsystem::EndSession()
{
    if (Session == nullptr)
        return;
    
    bool bPlayInEditor = GEditor && GEditor->PlayWorld;
    
    gekko_destroy(&Session);
    if (bPlayInEditor)
    {
        GekkoNetLocalAdapter::EmptyAddresses();
    }
    else
    {
        gekko_default_adapter_destroy();
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
    // Check if we need to catch up and frame skip timer hasn't triggered
    // if the FrameSkipTimer has updated we refrain from forcing another frame skip until we should.
    const bool CatchUp = NeedToCatchUp() && (FrameSkipTimer == 0);
    StepLogic();
    
    // run an additional frame if we need to catch up.
    if (CatchUp) {
        StepLogic();
        FrameSkipTimer = FRAME_SKIP_TIMER_MAX;
    }

    FrameSkipTimer -= 1;
    FrameSkipTimer = FMath::Max(FrameSkipTimer, 0);
}

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
                UE_LOG(LogGekkoNet, Log, TEXT("Player %d is syncing."), SyncHandle);
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
                // UE_LOG(LogGekkoNet, Log, TEXT("Gekko save called! (Frame:%d Checksum:0x%08X"), Ev->data.save.frame, *Ev->data.save.checksum);
                break;
            }
        case GekkoLoadEvent:
            {
                SimHost->GekkoLoad(Ev);
                // UE_LOG(LogGekkoNet, Log, TEXT("Gekko load called! (Frame:%d"), Ev->data.load.frame);
                break;
            }
        case GekkoAdvanceEvent:
            {
                const bool rolling_back = Ev->data.adv.rolling_back;
                SimHost->GekkoAdvance(Ev);
                FramesRolledBack += rolling_back ? 1 : 0;
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
    if (StatsUpdateTimer == 0) {
        GekkoNetworkStats GNetStats;
        gekko_network_stats(Session, Player, &GNetStats);

        NetStats.Ping = GNetStats.avg_ping;
        NetStats.Delay = LocalDelay;

        if (FrameMaxRollback < NetStats.Rollback) {
            // Don't decrease the reading by more than a frame to account for
            // the opponent not pressing buttons for 1-2 seconds
            NetStats.Rollback -= 1;
        } else {
            NetStats.Rollback = FrameMaxRollback;
        }

        FrameMaxRollback = 0;
        StatsUpdateTimer = STATS_UPDATE_TIMER_MAX;
    }

    StatsUpdateTimer -= 1;
    StatsUpdateTimer =  FMath::Max(StatsUpdateTimer, 0);
    
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

void UGekkoNetSubsystem::SetLocalAdapter(int32 Index)
{
    LocalAdapterID = Index;
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
