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
    
    GekkoConfig config{};
    
    // default
    config.input_size = Config.SessionSize.InputSize;
    config.state_size = Config.SessionSize.StateSize;
    config.input_prediction_window = Config.InputPredictionWindow;
    config.num_players = Config.Players.Num();
    
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

    for (int i = 0; i < Config.Players.Num(); ++i)
    {
        FGekkoPlayerPeer Player = Config.Players[i];
        if (Player.PlayerType == EGekkoPlayerType::LocalPlayer)
        {
            gekko_add_actor(Session, GekkoLocalPlayer, nullptr);
            gekko_set_local_delay(Session, i, Player.LocalInputDelay + Config.FramesRunahead);
        }
        else
        {
            GekkoNetAddress addr = {};
            
            FString AddressStr = FString::Printf(TEXT("127.0.0.1:%d"), Player.RemoteInfo.RemotePort);
            auto AnsiStr = StringCast<ANSICHAR>(*AddressStr);
            addr.data = (void*)AnsiStr.Get();
            addr.size = AnsiStr.Length();
            
            bool bIsSpectating = Player.PlayerType == EGekkoPlayerType::Spectator;
            gekko_add_actor(Session, bIsSpectating ? GekkoSpectator : GekkoRemotePlayer, &addr);
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
        Config.FramesRunahead = NewConfig.FramesRunahead;
        gekko_set_runahead(Session, Config.FramesRunahead);
        
        for (int i = 0; i < Config.Players.Num(); i++)
        {
            if (Config.Players[i].PlayerType == EGekkoPlayerType::LocalPlayer)
            {
                Config.Players[i].LocalInputDelay = NewConfig.Players[i].LocalInputDelay;
                gekko_set_local_delay(Session, i, Config.Players[i].LocalInputDelay);
            }
        }
    }
    else
    {
        Config = NewConfig;
    }
}

bool UGekkoNetSubsystem::SetLocalDelay(int32 PlayerIndex, int32 Delay)
{
    bool bChangedOccured = false;
    FGekkoSessionConfig UpdateConfig = Config;
    for (int i = 0; i < UpdateConfig.Players.Num(); i++)
    {
        if (i == PlayerIndex)
        {
            if (UpdateConfig.Players[i].PlayerType == EGekkoPlayerType::LocalPlayer)
            {
                UpdateConfig.Players[i].LocalInputDelay = Delay;
                bChangedOccured = true;
                break;
            }
        }
    }

    if (bChangedOccured)
    {
        SetSessionConfig(UpdateConfig);
        return true;
    }
    return false;
}

bool UGekkoNetSubsystem::SetLocalDelayForAllPlayers(int32 Delay)
{
    bool bChangedOccured = false;
    FGekkoSessionConfig UpdateConfig = Config;
    for (int i = 0; i < UpdateConfig.Players.Num(); i++)
    {
        if (UpdateConfig.Players[i].PlayerType == EGekkoPlayerType::LocalPlayer)
        {
            bChangedOccured = true;
            UpdateConfig.Players[i].LocalInputDelay = Delay;
        }
    }
    
    if (bChangedOccured)
    {
        SetSessionConfig(UpdateConfig);
        return true;
    }
    return false;
}

bool UGekkoNetSubsystem::SetRunahead(int32 Runahead)
{
    FGekkoSessionConfig UpdateConfig = Config;

    if (UpdateConfig.FramesRunahead != Runahead)
    {
        UpdateConfig.FramesRunahead = Runahead;
        SetSessionConfig(UpdateConfig);
        return true;
    }
    return false;
}

float UGekkoNetSubsystem::GetFramesAhead() const
{
    if (!IsSessionActive())
    {
        return 0.f;
    }
    return gekko_frames_ahead(Session);
}

bool UGekkoNetSubsystem::GetNetworkStats(int32 PlayerIndex, FGekkoNetworkStats& OutStats) const
{
    if (!IsSessionActive())
    {
        return false;
    }
    GekkoNetworkStats gekkostats{};
    gekko_network_stats(Session, PlayerIndex, &gekkostats);
    OutStats.KbSent     = gekkostats.kb_sent;
    OutStats.KbReceived = gekkostats.kb_received;
    OutStats.LastPing   = gekkostats.last_ping;
    OutStats.AvgPing    = gekkostats.avg_ping;
    OutStats.Jitter     = gekkostats.jitter;
    return true;
}

float UGekkoNetSubsystem::GetPlayerPing(int32 PlayerIndex) const
{
    if (!IsSessionActive())
    {
        return 0.f;
    }
    GekkoNetworkStats gekkostats{};
    gekko_network_stats(Session, PlayerIndex, &gekkostats);
    return gekkostats.avg_ping;
}

bool UGekkoNetSubsystem::IsPlayerType(EGekkoPlayerType Type, int32 PlayerIndex)
{
    return Config.Players[PlayerIndex].PlayerType == Type;
}

bool UGekkoNetSubsystem::IsLocalPlayer(int32 PlayerIndex)
{
    return IsPlayerType(EGekkoPlayerType::LocalPlayer, PlayerIndex);
}

bool UGekkoNetSubsystem::IsRemotePlayer(int32 PlayerIndex)
{
    return IsPlayerType(EGekkoPlayerType::RemotePlayer, PlayerIndex);
}

bool UGekkoNetSubsystem::IsSpectator(int32 PlayerIndex)
{
    return IsPlayerType(EGekkoPlayerType::Spectator, PlayerIndex);
}
