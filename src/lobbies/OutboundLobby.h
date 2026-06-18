#pragma once

#include <umbrellas/include-glm.h>

#include "CardCorridorLobby.h"

// The outbound lobby (the hub): the same card corridor as the memory palace, but
// populated with the outbound screenshots (the Cursor-founder DMs etc.). Tinted a
// little cooler so the two lobbies feel distinct.
class OutboundLobby : public CardCorridorLobby {
public:
    explicit OutboundLobby(App* app);

protected:
    auto BackbufferClear() const -> glm::vec3 override { return {0.015f, 0.020f, 0.030f}; }
    auto WorldColor() const -> glm::vec3 override { return {0.015f, 0.020f, 0.030f}; }
};
