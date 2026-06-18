#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <umbrellas/include-glm.h>

#include "Lobby.h"

class BeProp;
class BeMesh;
class BeMaterial;
class BeShader;

// A corridor of glowing cards lining a grid-floor walkway, with a follow-camera
// toggle puck, bloom, and drag-drop to add new cards. This is the shared "look" of
// every zeen lobby — concrete lobbies (memory palace, outbound, ...) subclass it
// and just supply their own cards directory + portals + spawn.
//
// A direct Lobby (its own pass stack: custom sleeve pass + bloom).
class CardCorridorLobby : public Lobby {
public:
    // `cardsDir` is the asset subdir under assets/lobbies/<name>/ that holds the
    // initial card images (usually "cards").
    CardCorridorLobby(App* app, std::string name, std::string cardsDir = "cards");
    ~CardCorridorLobby() override;

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

protected:
    // Backbuffer clear + ambient/world tint so different lobbies can feel distinct
    // while sharing the corridor structure. Override in subclasses.
    virtual auto BackbufferClear() const -> glm::vec3 { return {0.018f, 0.018f, 0.020f}; }
    virtual auto AmbientColor() const -> glm::vec3 { return glm::vec3(0.82f); }
    virtual auto WorldColor() const -> glm::vec3 { return {0.018f, 0.018f, 0.020f}; }

    std::string _cardsDir;

private:
    // Per-lobby render-target name (lobbies share the texture registry, so the
    // names must be distinct). e.g. memory-palace -> "memory-palace_HDR".
    auto Tex(const char* suffix) const -> std::string { return _name + "_" + suffix; }

    auto AddCardFromImage(const std::filesystem::path& imagePath) -> void;
    auto SubmitFrame(float now) -> void;

    std::shared_ptr<BeMaterial> _uniformMaterial;
    std::shared_ptr<BeProp> _floor;
    std::shared_ptr<BeProp> _buttonBase;
    std::shared_ptr<BeProp> _buttonKnob;
    std::shared_ptr<BeProp> _portalMarker;   // glowing pillar at each portal

    std::shared_ptr<BeMesh> _cardMesh;
    std::shared_ptr<BeMesh> _sleeveMesh;
    std::shared_ptr<BeMesh> _buttonMesh;

    std::weak_ptr<BeShader> _cardShader;
    std::weak_ptr<BeShader> _sleeveShader;

    std::vector<MemoryCard> _cards;
    bool _cardsFollowCamera = true;
    bool _prepared = false;
};
