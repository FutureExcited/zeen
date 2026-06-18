#include "MemoryPalaceLobby.h"

MemoryPalaceLobby::MemoryPalaceLobby(App* app)
    : CardCorridorLobby(app, "memory-palace", "cards") {
    _defaultSpawnPos = {0.0f, 1.75f, -6.0f};
    _defaultSpawnLook = {0.0f, 1.95f, 0.0f};
    // Teleporter to outbound, directly BEHIND the spawn (spawn faces +Z down the
    // corridor, so behind is more -Z).
    _portals.push_back({.TargetLobby = "outbound", .Position = {0.0f, 0.0f, -9.0f}, .Radius = 1.5f, .Label = "Outbound"});
}
