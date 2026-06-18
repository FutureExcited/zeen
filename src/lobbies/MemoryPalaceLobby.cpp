#include "MemoryPalaceLobby.h"

MemoryPalaceLobby::MemoryPalaceLobby(App* app)
    : CardCorridorLobby(app, "memory-palace", "cards") {
    _defaultSpawnPos = {0.0f, 1.75f, -6.0f};
    _defaultSpawnLook = {0.0f, 1.95f, 0.0f};
    // Teleporter to outbound, right beside the spawn.
    _portals.push_back({.TargetLobby = "outbound", .Position = {2.5f, 0.0f, -6.0f}, .Radius = 1.4f, .Label = "Outbound"});
}
