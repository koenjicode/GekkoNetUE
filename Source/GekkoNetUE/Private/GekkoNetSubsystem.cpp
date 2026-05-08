// Fill out your copyright notice in the Description page of Project Settings.

#include "GekkoNetSubsystem.h"
#include "GekkoNetLog.h"
#include "GekkoNetSimulationInterface.h"

#define STATS_UPDATE_TIMER_MAX 60
#define FRAME_SKIP_TIMER_MAX 60

void UGekkoNetSubsystem::StartGekko(FGekkoSessionConfig Config, TScriptInterface<IGekkoNetSimulationInterface> NewHost, int32 PlayerIndex)
{
    if (Config.NumPlayers < 2 || Config.InputSize <= 0 || Config.StateSize <= 0)
    {
        UE_LOG(LogGekkoNet, Error, TEXT("Bad config created, failed to start session."));
        return;
    }
    
    if (SimHost.GetObject() == nullptr)
    {
        if (NewHost == nullptr)
        {
            UE_LOG(LogGekkoNet, Error, TEXT("No host present for Gekko to run, failed to start session."));
            return;
        }
        SetSimulationHost(NewHost);
    }

    if (PlayerID < 0)
    {
        if (PlayerIndex < 0 || PlayerIndex > Config.NumPlayers)
        {
            UE_LOG(LogGekkoNet, Error, TEXT("Invalid player index chosen, failed to start session."));
            return;
        }
        PlayerID = PlayerIndex;
    }
    
    GekkoConfig GekkoConfig = {};
    FMemory::Memzero(&GekkoConfig, sizeof(GekkoConfig));
    
    NumPlayers = GekkoConfig.num_players = Config.NumPlayers;
    InputSize = GekkoConfig.input_size = Config.InputSize;
    StateSize = GekkoConfig.state_size = Config.StateSize;
    
    GekkoConfig.max_spectators = 0;
    GekkoConfig.input_prediction_window = Config.InputPredictionWindow;

#if defined(GEKKO_DEBUG)
    GekkoConfig.desync_detection = true;
#endif

    if (gekko_create(&Session, GekkoGameSession)) {
        gekko_start(Session, &GekkoConfig);
    } else
    {
        UE_LOG(LogGekkoNet, Error, TEXT("Session is already running, failed to start a new one."));
        return;
    }
    
    if (PlayerID == 0)
    {
        LocalPort = 50000;
        RemotePort = 50001;
    }
    else
    {
        LocalPort = 50001;
        RemotePort = 50000;
    }
    
    gekko_net_adapter_set(Session, gekko_default_adapter(LocalPort));
    
    UE_LOG(LogGekkoNet, Log, TEXT("Starting a session for player %d at port %hu\n"), PlayerID, LocalPort);
            
    FString AddressString = FString::Printf(TEXT("127.0.0.1:%d"), RemotePort);
    auto Anistr = StringCast<ANSICHAR>(*AddressString);

    GekkoNetAddress RemoteAddress;
    RemoteAddress.data = (void*)Anistr.Get();
    RemoteAddress.size = Anistr.Length();

    for (int i = 0; i < GekkoConfig.num_players; i++) {
        const bool LocalPlayer = (i == PlayerID);
        if (LocalPlayer) {
            PlayerHandle = gekko_add_actor(Session, GekkoLocalPlayer, NULL);
            gekko_set_local_delay(Session, PlayerHandle, LocalDelay);
        } else {
            gekko_add_actor(Session, GekkoRemotePlayer, &RemoteAddress);
        }
    }
    SessionState = EGekkoSessionState::Connecting;
}

void UGekkoNetSubsystem::StartGekko(FGekkoSessionConfig Config, int32 PlayerIndex)
{
    StartGekko(Config, nullptr, PlayerIndex);
}

void UGekkoNetSubsystem::ShutdownGekko()
{
    if (Session != nullptr)
    {
        gekko_destroy(&Session);
        gekko_default_adapter_destroy();
        SessionState = EGekkoSessionState::Idling;
        
        UE_LOG(LogGekkoNet, Warning, TEXT("Closing session for player %d at port %hu\n"), PlayerID, LocalPort);
        
        PlayerID = INDEX_NONE;
        PlayerHandle = INDEX_NONE;
        
        LocalPort = 0;
        RemotePort = 0;
        
        NumPlayers = 0;
        InputSize = 0;
        StateSize = 0;
    }
}

void UGekkoNetSubsystem::UpdateGekko()
{
    switch (SessionState)
    {
    case EGekkoSessionState::Idling:
        break;
    case EGekkoSessionState::Transitioning:
        break;
    case EGekkoSessionState::Connecting:
    case EGekkoSessionState::Running:
        RunNetplay();
        break;
    case EGekkoSessionState::Exiting:
        ShutdownGekko();
        break;
    }
}

void UGekkoNetSubsystem::RunNetplay()
{
    // Check if we need to catch up and frame skip timer hasn't triggered
    // if the FrameSkipTimer has updated we refrain from forcing another frame skip until we should.
    const bool CatchUp = NeedToCatchUp() && (FrameSkipTimer == 0);
    StepLogic(!CatchUp);
    
    // run an additional frame if we need to catch up.
    if (CatchUp) {
        StepLogic(true);
        FrameSkipTimer = FRAME_SKIP_TIMER_MAX;
    }

    FrameSkipTimer -= 1;
    FrameSkipTimer = FMath::Max(FrameSkipTimer, 0);

    // Update stats
    UpdateNetworkStats();
}

void UGekkoNetSubsystem::HandleDisconnection(GekkoSessionEvent* Ev)
{
    FramesBehind = -gekko_frames_ahead(Session);
    if (SessionState == EGekkoSessionState::Exiting || SessionState == EGekkoSessionState::Idling) {
        return;
    }
    SimHost->GekkoDisconnect(Ev);
    OnPlayerDisconnected.Broadcast(Ev->data.disconnected.handle);
    SessionState = EGekkoSessionState::Exiting;
}

void UGekkoNetSubsystem::StepLogic(bool bShouldDraw)
{
    ProcessSession();
    ProcessEvents(bShouldDraw);
}

void UGekkoNetSubsystem::ProcessSession()
{
    FramesBehind = -gekko_frames_ahead(Session);
    
    gekko_network_poll(Session);
    
    TArray<uint8> input_data;
    input_data.SetNumZeroed(InputSize);

    SimHost->GekkoGetLocalInputs(input_data.GetData());
    gekko_add_local_input(Session, PlayerID, input_data.GetData());

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
                SessionState = EGekkoSessionState::Running;
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

void UGekkoNetSubsystem::ProcessEvents(bool bShouldDraw)
{
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
                UE_LOG(LogGekkoNet, Log, TEXT("Gekko save called! (Frame:%d Checksum:0x%08X"), Ev->data.save.frame, *Ev->data.save.checksum);
                break;
            }
        case GekkoLoadEvent:
            {
                SimHost->GekkoLoad(Ev);
                UE_LOG(LogGekkoNet, Log, TEXT("Gekko load called! (Frame:%d"), Ev->data.load.frame);
                break;
            }
        case GekkoAdvanceEvent:
            {
                const bool rolling_back = Ev->data.adv.rolling_back;
                SimHost->GekkoAdvance(Ev, bShouldDraw && !rolling_back);
                FramesRolledBack += rolling_back ? 1 : 0;
                break;
            }
        default:
            break;
        }
    }
    FrameMaxRollback = FMath::Max(FrameMaxRollback, FramesRolledBack);
}

void UGekkoNetSubsystem::UpdateNetworkStats()
{
    if (StatsUpdateTimer == 0) {
        GekkoNetworkStats GNetStats;
        gekko_network_stats(Session, PlayerID ^ 1, &GNetStats);

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
}

FGekkoFullNetworkStats UGekkoNetSubsystem::GetFullNetworkStats() const
{
    GekkoNetworkStats GNetStats;
    gekko_network_stats(Session, PlayerID ^ 1, &GNetStats);
    
    FGekkoFullNetworkStats FullNetStats;
    FullNetStats.AvgPing = GNetStats.avg_ping;
    FullNetStats.Jitter = GNetStats.jitter;
    FullNetStats.KbReceived = GNetStats.kb_received;
    FullNetStats.KbSent = GNetStats.kb_sent;
    
    return FullNetStats;
}

void UGekkoNetSubsystem::SetPlayerID(int32 NewID)
{
    PlayerID = NewID;
}

bool UGekkoNetSubsystem::SetSimulationHost(TScriptInterface<IGekkoNetSimulationInterface> NewHost)
{
    SimHost = NewHost;
    return SimHost.GetObject() ? true : false;
}

bool UGekkoNetSubsystem::SetLocalDelay(int32 Delay, int32 LocalPlayerIndex)
{
    LocalDelay = FMath::Max(Delay, 0);
    if (Session != nullptr && LocalPlayerIndex >= 0 && LocalPlayerIndex < NumPlayers)
    {
        gekko_set_local_delay(Session, LocalPlayerIndex, LocalDelay);
        UE_LOG(LogGekkoNet, Log, TEXT("Gekko Delay has been updated to %d for Player %d."), LocalDelay, LocalPlayerIndex);
    }
    return true;
}

bool UGekkoNetSubsystem::SetLocalDelay(int32 Delay)
{
    return SetLocalDelay(Delay, -1);
}

bool UGekkoNetSubsystem::SetRunahead(int32 Runahead)
{
    LocalRunahead = FMath::Max(Runahead, 0);
    if (Session != nullptr)
    {
        gekko_set_runahead(Session, LocalRunahead);
        UE_LOG(LogGekkoNet, Log, TEXT("Gekko Runahead has been updated to %d."), LocalRunahead);
    }
    return false;
}

bool UGekkoNetSubsystem::NeedToCatchUp() const
{
    return FramesBehind >= 1;
}
