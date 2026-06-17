#pragma once

#include <memory>
#include <string>
#include <vector>

#include <umbrellas/include-glm.h>

#include "Lobby.h"
#include "UploadSpawner.h"

class BeProp;
class BeMaterial;
class BeShader;

// Shared scaffolding for a PBR-lit walkable room. Handles:
//   - render machine + camera + uniform material
//   - the deferred G-buffer / lighting / FXAA pass stack (rebuilt on entry)
//   - a floor plane
//   - upload spawning wiring (drag-drop -> object in the room)
//   - per-frame camera uniforms + sun/fill lights
//
// Concrete lobbies override BuildContent() to load their own props and
// SubmitContent() to submit those props each frame. The PBR shader set
// (zoro-pbr + the standard fullscreen/light shaders) is shared via the
// per-lobby `shaders/` asset folder.
class PbrRoomLobby : public Lobby {
public:
    PbrRoomLobby(App* app, std::string name);

    auto Prepare() -> void override;
    auto OnEnter() -> void override;
    auto Tick(float deltaTime) -> void override;

protected:
    // Subclass hooks.
    virtual auto BuildContent() -> void {}          // load props during Prepare()
    virtual auto SubmitContent(float now) -> void {} // submit props during Tick()
    virtual auto FloorColor() const -> glm::vec3 { return {0.24f, 0.24f, 0.25f}; }
    virtual auto AmbientColor() const -> glm::vec3 { return glm::vec3(0.95f); }

    // The G-buffer target name prefix, e.g. "Zoro" -> "Zoro_HDR". Distinct per
    // lobby so machines don't collide in the shared texture registry.
    virtual auto TargetPrefix() const -> std::string = 0;

    auto Tex(const std::string& suffix) const -> std::string { return TargetPrefix() + "_" + suffix; }

    // Shared per-frame lights; override to customise.
    virtual auto SubmitLights() -> void;

    // Helpers for subclasses.
    auto Shader() const -> std::weak_ptr<BeShader>;

    std::shared_ptr<BeMaterial> _uniformMaterial;
    std::shared_ptr<BeProp> _floor;
    std::unique_ptr<UploadSpawner> _uploads;
    std::weak_ptr<BeShader> _shader;
    bool _prepared = false;
};
