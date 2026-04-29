// Fill out your copyright notice in the Description page of Project Settings.

#include "GekkoNetSubsystem.h"
#include "GekkoNetUnrealAdapter.h"
#include "Cooker/CookDependency.h"

void UGekkoNetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGekkoNetSubsystem::Deinitialize()
{
    DestroySession();
	Super::Deinitialize();
}

bool UGekkoNetSubsystem::CreateSession(int32 LocalPort, bool AsSpectator)
{
    DestroySession();
    
    int32 num_players = Config.GetNumberOfParticipants();
    
    GekkoConfig config{};
    
    // default
    config.input_size = Config.SessionSize.InputSize;
    config.state_size = Config.SessionSize.StateSize;
    config.input_prediction_window = Config.InputPredictionWindow;
    config.num_players = num_players;
    
    // spectators
    config.max_spectators = Config.MaxSpectators;
    config.spectator_delay = Config.SpectatorDelay;
    
    // stress test
    config.check_distance = Config.CheckDistance;
    
    // debugging (mostly)
    config.desync_detection = Config.DesyncDetection;
    
    // Should be primarily used in debugging scenarios, might be worth removing this from the session config.
    bool session_created = gekko_create(&Session, AsSpectator ? GekkoSpectateSession : GekkoGameSession);
    
    if (!session_created)
    {
        return false;
    }
    
    // TODO: current only supporting raw local ip connections, but the net adapter will need to be updated
    gekko_start(Session, &config);
    gekko_net_adapter_set(Session, FGekkoNetAdapter::UE_Gekko_Adapter(LocalPort));
    
    gekko_set_runahead(Session, Config.FramesRunahead);
    
    // loop through the num of players in the game, adding them to gekko locally or remotely.
    for (int i = 0; i < num_players; i++) {
        bool is_local = false;
        for (int j = 0; j < Config.LocalPlayers.Num(); j++) {
            if (Config.LocalPlayers[j].PlayerIndex == i) {
                is_local = true;
                break;
            }
        }

        if (is_local) {
            int local_delay = 1;
            for (int j = 0; j < Config.LocalPlayers.Num(); j++) {
                if (Config.LocalPlayers[j].PlayerIndex == i) {
                    local_delay = Config.LocalPlayers[j].LocalInputDelay;
                    break;
                }
            }
            gekko_add_actor(Session, GekkoLocalPlayer, nullptr);
            gekko_set_local_delay(Session, i, local_delay + Config.FramesRunahead);
        }
        else {
            int remote_port = 0;
            for (int j = 0; j < Config.RemotePlayers.Num(); j++) {
                if (Config.RemotePlayers[j].PlayerIndex == i) {
                    remote_port = Config.RemotePlayers[j].Port;
                    break;
                }
            }
            GekkoNetAddress addr = {};
            
            // Build the address as FString
            FString AddressStr = FString::Printf(TEXT("127.0.0.1:%d"), remote_port);

            // Convert at the GekkoNet boundary
            auto AnsiStr = StringCast<ANSICHAR>(*AddressStr);
            addr.data = (void*)AnsiStr.Get();
            addr.size = AnsiStr.Length();
            
            gekko_add_actor(Session, GekkoRemotePlayer, &addr);
        }
    }

    return true;
}

void UGekkoNetSubsystem::DestroySession()
{
    if (IsSessionActive())
    {
        FGekkoNetAdapter::UE_Gekko_Adapter_Destroy();
        gekko_destroy(&Session);
        
        Config = FGekkoSessionConfig();
    }
}

void UGekkoNetSubsystem::SetSessionConfig(FGekkoSessionConfig NewConfig)
{
    if (IsSessionActive())
    {
        // Ignore updating if a session is actively running.
        return;
    }
    Config = NewConfig;
}

bool UGekkoNetSubsystem::SetLocalDelay(int32 Player, int32 Delay)
{
    if (!IsSessionActive())
    {
        return false;
    }
    for (int i = 0; i < Config.LocalPlayers.Num(); i++)
    {
        if (Config.LocalPlayers[i].PlayerIndex == Player)
        {
            Config.LocalPlayers[i].LocalInputDelay = Delay;
            break;
        }
    }
    gekko_set_local_delay(Session, Player, Delay);
    return true;
}

bool UGekkoNetSubsystem::SetRunahead(int32 Runahead)
{
    if (!IsSessionActive())
    {
        return false;
    }
    Config.FramesRunahead = Runahead;
    gekko_set_runahead(Session, Runahead);
    return true;
}

bool UGekkoNetSubsystem::GetNetworkStats(int32 PlayerHandle, FGekkoNetworkStats& OutStats) const
{
    if (!IsSessionActive())
    {
        return false;
    }
    GekkoNetworkStats gekkostats{};
    gekko_network_stats(Session, PlayerHandle, &gekkostats);
    OutStats.KbSent     = gekkostats.kb_sent;
    OutStats.KbReceived = gekkostats.kb_received;
    OutStats.LastPing   = gekkostats.last_ping;
    OutStats.AvgPing    = gekkostats.avg_ping;
    OutStats.Jitter     = gekkostats.jitter;
    return true;
}

float UGekkoNetSubsystem::GetFramesAhead() const
{
    if (!IsSessionActive())
    {
        return 0.f;
    }
    return gekko_frames_ahead(Session);
}

float UGekkoNetSubsystem::GetPlayerPing(int32 Player) const
{
    if (!IsSessionActive())
    {
        return 0.f;
    }
    GekkoNetworkStats gekkostats{};
    gekko_network_stats(Session, Player, &gekkostats);
    return gekkostats.avg_ping;
}
