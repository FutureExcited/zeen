#pragma once

#include <memory>
#include <vector>

#include "PbrRoomLobby.h"

class BeProp;

// The memory palace lobby. Minimal for now (a room with floating cards is the
// full version, ported separately); reuses the shared PBR room scaffolding.
class MemoryPalaceLobby : public PbrRoomLobby {
public:
    explicit MemoryPalaceLobby(App* app);

protected:
    auto TargetPrefix() const -> std::string override { return "Memory"; }
    auto FloorColor() const -> glm::vec3 override { return {0.10f, 0.10f, 0.13f}; }
    auto AmbientColor() const -> glm::vec3 override { return glm::vec3(0.6f); }
    auto BuildContent() -> void override;
    auto SubmitContent(float now) -> void override;

private:
    std::shared_ptr<BeProp> _plinth;
};
