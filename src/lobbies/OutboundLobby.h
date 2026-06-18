#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <umbrellas/include-glm.h>

#include "PbrRoomLobby.h"

class BeProp;

// The "outbound" lobby — the hub. A gallery of screenshots (the outbound DMs /
// posts) standing on the floor, plus drag-drop to add more. Walk into the portal
// to cross to the memory palace.
class OutboundLobby : public PbrRoomLobby {
public:
    explicit OutboundLobby(App* app);

protected:
    auto TargetPrefix() const -> std::string override { return "Outbound"; }
    auto FloorColor() const -> glm::vec3 override { return {0.15f, 0.16f, 0.19f}; }
    auto AmbientColor() const -> glm::vec3 override { return glm::vec3(0.8f); }
    auto BuildContent() -> void override;
    auto SubmitContent(float now) -> void override;
    auto SubmitLights() -> void override;

private:
    struct Panel {
        std::shared_ptr<BeProp> prop;
        glm::vec3 position;
        glm::vec3 scale;
        std::string name;
    };
    auto AddScreenshot(const std::filesystem::path& path) -> void;

    std::vector<Panel> _panels;
};
