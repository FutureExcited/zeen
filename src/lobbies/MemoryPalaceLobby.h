#pragma once

#include "CardCorridorLobby.h"

// The memory palace: the card corridor populated with the LinkedIn / X / YC cards.
class MemoryPalaceLobby : public CardCorridorLobby {
public:
    explicit MemoryPalaceLobby(App* app);
};
