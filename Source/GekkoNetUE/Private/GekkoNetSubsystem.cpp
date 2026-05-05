// Fill out your copyright notice in the Description page of Project Settings.

#include "GekkoNetSubsystem.h"
#include "GekkoNetLog.h"
#include "GekkoNetSimulationInterface.h"

#define STATS_UPDATE_TIMER_MAX 60
#define FRAME_SKIP_TIMER_MAX 60 // Allow skipping a frame roughly every second

void UGekkoNetSubsystem::StartGekko(FGekkoSessionConfig Config)
{
    GekkoConfig config = {};
    FMemory::Memzero(&config, sizeof(GekkoConfig));
    
    NumPlayers = config.num_players = Config.NumPlayers;
    InputSize = config.input_size = Config.InputSize;
    StateSize = config.state_size = Config.StateSize;
    
    config.max_spectators = 0;
    config.input_prediction_window = Config.InputPredictionWindow;

#if defined(DESYNC_TESTING)
    config.desync_detection = true;
#endif

    if (gekko_create(&Session, GekkoGameSession)) {
        gekko_start(Session, &config);
    } else
    {
        UE_LOG(LogGekkoNet, Error, TEXT("Session is already running, failed to start a new one."));
    }
    
    if (PlayerNumber == 0)
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
    
    UE_LOG(LogGekkoNet, Log, TEXT("Starting a session for player %d at port %hu\n"), PlayerNumber, LocalPort);
            
    FString AddressString = FString::Printf(TEXT("127.0.0.1:%d"), RemotePort);
    auto Anistr = StringCast<ANSICHAR>(*AddressString);

    GekkoNetAddress RemoteAddress;
    RemoteAddress.data = (void*)Anistr.Get();
    RemoteAddress.size = Anistr.Length();

    for (int i = 0; i < config.num_players; i++) {
        const bool is_local_player = (i == PlayerNumber);

        if (is_local_player) {
            PlayerHandle = gekko_add_actor(Session, GekkoLocalPlayer, NULL);
            gekko_set_local_delay(Session, PlayerHandle, DEFAULT_INPUT_DELAY);
        } else {
            gekko_add_actor(Session, GekkoRemotePlayer, &RemoteAddress);
        }
    }
    
    SessionState = EGekkoSessionState::Connecting;
}


void UGekkoNetSubsystem::ShutdownGekko()
{
    if (Session != nullptr)
    {
        gekko_destroy(&Session);
        gekko_default_adapter_destroy();
        SessionState = EGekkoSessionState::Idling;
    }
}

void UGekkoNetSubsystem::UpdateNetplay()
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
    const bool catch_up = NeedToCatchUp() && (FrameSkipTimer == 0);
    StepLogic(!catch_up);
    
    // run an additional frame if we need to catch up.
    if (catch_up) {
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
    IGekkoNetSimulationInterface* Host = SimulationHost ? Cast<IGekkoNetSimulationInterface>(SimulationHost.GetObject()) : nullptr;
    FramesBehind = -gekko_frames_ahead(Session);
    if (SessionState == EGekkoSessionState::Exiting || SessionState == EGekkoSessionState::Idling) {
        return;
    }
    Host->GekkoDisconnect(Ev);
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
    IGekkoNetSimulationInterface* Host = SimulationHost ? Cast<IGekkoNetSimulationInterface>(SimulationHost.GetObject()) : nullptr;
    FramesBehind = -gekko_frames_ahead(Session);
    
    gekko_network_poll(Session);
    
    TArray<uint8> input_data;
    input_data.SetNumZeroed(InputSize);

    Host->GekkoGetLocalInputs(input_data.GetData());
    gekko_add_local_input(Session, PlayerNumber, input_data.GetData());

    int session_event_count = 0;
    GekkoSessionEvent** session_events = gekko_session_events(Session, &session_event_count);
    for (int i = 0; i < session_event_count; ++i)
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
    IGekkoNetSimulationInterface* Host = SimulationHost ? Cast<IGekkoNetSimulationInterface>(SimulationHost.GetObject()) : nullptr;
    
    int event_count = 0;
    int frames_rolled_back = 0;
    GekkoGameEvent** Updates = gekko_update_session(Session, &event_count);
    for (int i = 0; i < event_count; ++i)
    {
        GekkoGameEvent* Ev = Updates[i];
        if (!Host)
        {
            continue;
        }

        switch (Ev->type)
        {
        case GekkoSaveEvent:
            {
                Host->GekkoSave(Ev);
                break;
            }
        case GekkoLoadEvent:
            {
                Host->GekkoLoad(Ev);
                break;
            }
        case GekkoAdvanceEvent:
            {
                const bool rolling_back = Ev->data.adv.rolling_back;
                Host->GekkoAdvance(Ev, bShouldDraw && !rolling_back);
                frames_rolled_back += rolling_back ? 1 : 0;
                break;
            }
        default:
            break;
        }
    }
}

void UGekkoNetSubsystem::UpdateNetworkStats()
{
    if (StatsUpdateTimer == 0) {
        GekkoNetworkStats net_stats;
        gekko_network_stats(Session, PlayerNumber ^ 1, &net_stats);

        NetStats.Ping = net_stats.avg_ping;
        NetStats.Delay = DEFAULT_INPUT_DELAY;

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

bool UGekkoNetSubsystem::SetLocalDelay(int32 LocalPlayer, int32 Delay)
{
    if (Session == nullptr)
    {
        return false;
    }
    gekko_set_local_delay(Session, LocalPlayer, Delay);
    return true;
}

bool UGekkoNetSubsystem::SetRunahead(int32 Runahead)
{
    if (Session == nullptr)
    {
        return false;
    }
    gekko_set_runahead(Session, Runahead);
    return false;
}

bool UGekkoNetSubsystem::NeedToCatchUp() const
{
    return FramesBehind >= 1;
}
