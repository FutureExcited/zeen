#pragma once

#include <memory>

#include "PbrRoomLobby.h"

class BeProp;

// The zoro lobby: a character model on the floor in a small room.
class ZoroLobby : public PbrRoomLobby {
public:
    explicit ZoroLobby(App* app);

protected:
    auto TargetPrefix() const -> std::string override { return "Zoro"; }
    auto FloorColor() const -> glm::vec3 override { return {0.24f, 0.24f, 0.25f}; }
    auto BuildContent() -> void override;
    auto SubmitContent(float now) -> void override;

private:
    std::shared_ptr<BeProp> _zoro;
    bool _hasModel = false;
};
