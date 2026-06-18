#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <umbrellas/include-glm.h>

#include "Lobby.h"

class BeProp;
class BeMesh;
class BeMaterial;
class BeShader;

// The memory palace: a corridor of glowing cards (LinkedIn / X / YC posts) lining
// a walkway, with a floor grid, a follow-camera toggle puck, bloom, and drag-drop
// to add new cards. Ported from the standalone example-memory-palace onto the
// BeScene-based lobby model. This is a direct Lobby (not a PbrRoomLobby) because it
// has its own pass stack (custom sleeve pass + bloom) and movement.
class MemoryPalaceLobby : public Lobby {
public:
    explicit MemoryPalaceLobby(App* app);
    ~MemoryPalaceLobby() override;

    auto Prepare() -> void override;
    auto OnEnter() -> void override;
    auto Tick(float deltaTime) -> void override;

    // Holds one card (front prop + glowing sleeve prop + transform).
    struct MemoryCard {
        std::shared_ptr<BeProp> Prop;
        std::shared_ptr<BeProp> SleeveProp;
        glm::vec3 Position;
        glm::quat Rotation;
        glm::vec3 Scale;
    };

private:
    auto AddCardFromImage(const std::filesystem::path& imagePath) -> void;
    auto SubmitFrame(float now) -> void;

    std::shared_ptr<BeMaterial> _uniformMaterial;
    std::shared_ptr<BeProp> _floor;
    std::shared_ptr<BeProp> _buttonBase;
    std::shared_ptr<BeProp> _buttonKnob;

    std::shared_ptr<BeMesh> _cardMesh;
    std::shared_ptr<BeMesh> _sleeveMesh;
    std::shared_ptr<BeMesh> _buttonMesh;

    std::weak_ptr<BeShader> _cardShader;
    std::weak_ptr<BeShader> _sleeveShader;

    std::vector<MemoryCard> _cards;
    bool _cardsFollowCamera = true;
    bool _prepared = false;
};
