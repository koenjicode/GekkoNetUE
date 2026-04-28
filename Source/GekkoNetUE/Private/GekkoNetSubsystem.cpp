// Fill out your copyright notice in the Description page of Project Settings.

#include "GekkoNetSubsystem.h"
#include "GekkoNetUnrealAdapter.h"

void UGekkoNetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGekkoNetSubsystem::Deinitialize()
{
    Destroy();
	Super::Deinitialize();
}

bool UGekkoNetSubsystem::Create(const FGekkoSessionConfig& Config, bool IsSpectator)
{
    Destroy();
    
    int32 num_players = Config.LocalPlayers.Num() + Config.RemotePlayers.Num();
    int32 local_port = Config.LocalPort;
    
    GekkoConfig config{};
    
    config.input_size = Config.SessionSize.InputSize;
    config.state_size = Config.SessionSize.StateSize;
    config.max_spectators = Config.MaxSpectators;
    config.input_prediction_window = Config.InputPredictionWindow;
    config.num_players = num_players;
    config.spectator_delay = Config.SpectatorDelay;
    
    config.desync_detection = Config.DesyncDetection;
    
    bool session_created = gekko_create(&session, IsSpectator ? GekkoSpectateSession : GekkoGameSession);
    
    if (!session_created)
    {
        return false;
    }
    
    // TODO: current only supporting raw local ip connections, but the net adapter will need to be updated
    gekko_start(session, &config);
    gekko_net_adapter_set(session, FGekkoNetAdapter::UE_Gekko_Adapter(local_port));
    
    gekko_set_runahead(session, Config.FramesRunahead);
    
    CurrentSessionConfig = Config;
    
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
            gekko_add_actor(session, GekkoLocalPlayer, nullptr);
            gekko_set_local_delay(session, i, local_delay + Config.FramesRunahead);
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
            
            gekko_add_actor(session, GekkoRemotePlayer, &addr);
        }
    }

    return true;
}

void UGekkoNetSubsystem::Destroy()
{
    if (IsSessionActive())
    {
        FGekkoNetAdapter::UE_Gekko_Adapter_Destroy();
        gekko_destroy(&session);
    }
}

bool UGekkoNetSubsystem::SetLocalDelay(int32 Player, int32 Delay)
{
    if (!IsSessionActive())
    {
        return false;
    }
    gekko_set_local_delay(session, Player, Delay);
    return true;
}

bool UGekkoNetSubsystem::SetRunahead(int32 Runahead)
{
    if (!IsSessionActive())
    {
        return false;
    }
    gekko_set_runahead(session, Runahead);
    return true;
}

bool UGekkoNetSubsystem::GetNetworkStats(int32 PlayerHandle, FGekkoNetworkStats& OutStats) const
{
    if (!IsSessionActive())
    {
        return false;
    }
    GekkoNetworkStats gekkostats{};
    gekko_network_stats(session, PlayerHandle, &gekkostats);
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
    return gekko_frames_ahead(session);
}
