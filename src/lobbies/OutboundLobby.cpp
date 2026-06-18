#include "OutboundLobby.h"

OutboundLobby::OutboundLobby(App* app)
    : CardCorridorLobby(app, "outbound", "cards") {
    _defaultSpawnPos = {0.0f, 1.75f, -6.0f};
    _defaultSpawnLook = {0.0f, 1.95f, 0.0f};
    // Teleporter to the memory palace, directly BEHIND the spawn (spawn faces +Z
    // down the corridor, so behind is more -Z).
    _portals.push_back({.TargetLobby = "memory-palace", .Position = {0.0f, 0.0f, -9.0f}, .Radius = 1.5f, .Label = "Memory Palace"});
}
