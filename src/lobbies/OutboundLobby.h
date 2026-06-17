#pragma once

#include <memory>

#include "PbrRoomLobby.h"

class BeProp;

// The "outbound" lobby — a clean staging room with a central pedestal. Walk back
// to where you came from with the portal or Backspace. A natural place to drop
// uploads onto the pedestal area.
class OutboundLobby : public PbrRoomLobby {
public:
    explicit OutboundLobby(App* app);

protected:
    auto TargetPrefix() const -> std::string override { return "Outbound"; }
    auto FloorColor() const -> glm::vec3 override { return {0.15f, 0.16f, 0.19f}; }
    auto AmbientColor() const -> glm::vec3 override { return glm::vec3(0.8f); }
    auto BuildContent() -> void override;
    auto SubmitContent(float now) -> void override;

private:
    std::shared_ptr<BeProp> _pedestal;
};
