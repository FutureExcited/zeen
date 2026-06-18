#pragma once

#include <memory>
#include <string>
#include <vector>

#include <umbrellas/include-glm.h>

#include <scenes/BeScene.h>

// Complete types needed: members hold unique_ptr<BeCamera> / unique_ptr<BeStandardRenderMachine>,
// whose destructors are instantiated across the lobby class hierarchy.
#include "BeCamera.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

class App;
class BeInput;

// A portal is a walkable spot on the floor that takes you to another lobby.
// Walk within Radius of Position and you transition to TargetLobby.
struct Portal {
    std::string TargetLobby;   // name of the lobby this portal leads to
    glm::vec3   Position{0.0f};
    float       Radius = 1.2f;
    std::string Label;         // shown in-world / on HUD
};

// Base class for every lobby. A lobby is a BeScene the manager can load/unload,
// but it also owns its own render machine + camera + content. The shared App
// (window/renderer/input) is injected so all lobbies draw into one window.
//
// Lifecycle:
//   Prepare()  - one-time heavy setup (index shaders, load props, build passes).
//                Called once at startup for every lobby.
//   OnLoad()   - called each time you walk INTO this lobby. Place the camera at
//                the right spawn (front door, or the portal you came back through).
//   Tick(dt)   - per-frame: move camera, check portals, submit geometry/lights.
//   OnUnload() - called when you leave.
class Lobby : public BeScene {
public:
    explicit Lobby(App* app, std::string name) : _app(app), _name(std::move(name)) {}
    ~Lobby() override;   // out-of-line: members hold unique_ptr to incomplete types

    auto Name() const -> const std::string& { return _name; }

    virtual auto Prepare() -> void {}
    auto OnLoad() -> void override;        // applies pending spawn, then OnEnter()
    virtual auto Tick(float deltaTime) -> void {}

    // Called by the manager just before OnLoad, telling us which portal/lobby the
    // player arrived from so we can spawn them at the matching return portal.
    auto SetArrivalFrom(const std::string& fromLobby) -> void { _arrivalFrom = fromLobby; }

    auto Portals() const -> const std::vector<Portal>& { return _portals; }

protected:
    // Subclasses override this for per-entry placement (camera spawn).
    virtual auto OnEnter() -> void {}

    // Shared Quake-style movement (walk/jump/bhop, double-tap-space to fly,
    // crouch, gravity, accel/friction) + mouselook. Identical across lobbies.
    auto UpdateFreeCamera(float deltaTime) -> void;

    // Per-lobby movement state (Quake-style controller).
    struct PlayerController {
        bool Flying = false;
        bool Grounded = true;
        float VerticalVelocity = 0.0f;
        float EyeHeight = 1.65f;
        double LastSpaceTapTime = -100.0;
        bool WaitingForFlightTap = false;
        bool SpaceReleasedAfterTap = true;
        bool SuppressSpaceClimbUntilReleased = false;
        glm::vec3 HorizontalVelocity{0.0f};
    };
    PlayerController _controller;

    // Walk-into-portal detection. If the camera is inside a portal radius, asks the
    // manager to switch lobbies. Returns true if a transition was requested.
    auto CheckPortals() -> bool;

    // Spawn the camera at the portal that leads to `fromLobby` (so walking "back"
    // drops you at the door you'd use to return). Falls back to _defaultSpawn.
    auto SpawnAtReturnPortalFrom(const std::string& fromLobby) -> void;

    App* _app = nullptr;
    std::string _name;
    std::string _arrivalFrom;   // lobby we just came from (set by manager)

    std::unique_ptr<BeCamera> _camera;
    std::unique_ptr<BeStandardRenderMachine> _machine;

    std::vector<Portal> _portals;
    glm::vec3 _defaultSpawnPos{0.0f, 1.8f, -5.0f};
    glm::vec3 _defaultSpawnLook{0.0f, 1.35f, 0.0f};

private:
    bool _portalCooldown = false;   // prevents instant re-trigger right after spawning on a portal
};
