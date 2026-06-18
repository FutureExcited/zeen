#include "OutboundLobby.h"

OutboundLobby::OutboundLobby(App* app)
    : CardCorridorLobby(app, "outbound", "cards") {
    _defaultSpawnPos = {0.0f, 1.75f, -6.0f};
    _defaultSpawnLook = {0.0f, 1.95f, 0.0f};
    // Teleporter to the memory palace, right beside the spawn.
    _portals.push_back({.TargetLobby = "memory-palace", .Position = {2.5f, 0.0f, -6.0f}, .Radius = 1.4f, .Label = "Memory Palace"});
}
