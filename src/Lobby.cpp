#include "Lobby.h"

#include <umbrellas/include-glfw.h>

#include "BeCamera.h"
#include "BeInput.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

#include "App.h"
#include "LobbyManager.h"

Lobby::~Lobby() = default;

auto Lobby::OnLoad() -> void {
    // Place the player. If we arrived from another lobby, spawn at the portal that
    // leads back there (so it feels like you walked through a door). Otherwise use
    // the lobby's default front-door spawn.
    if (!_arrivalFrom.empty()) {
        SpawnAtReturnPortalFrom(_arrivalFrom);
    } else if (_camera) {
        _camera->Position = _defaultSpawnPos;
    }
    _portalCooldown = true;   // don't re-trigger the portal we just spawned on
    OnEnter();
    _arrivalFrom.clear();
}

auto Lobby::SpawnAtReturnPortalFrom(const std::string& fromLobby) -> void {
    if (!_camera) return;
    for (const auto& portal : _portals) {
        if (portal.TargetLobby == fromLobby) {
            // Spawn a step in front of that portal, facing into the room.
            const glm::vec3 inward = glm::normalize(_defaultSpawnLook - portal.Position
                + glm::vec3(0.0001f));   // avoid zero vector
            _camera->Position = portal.Position - inward * 2.0f;
            _camera->Position.y = _defaultSpawnPos.y;
            return;
        }
    }
    _camera->Position = _defaultSpawnPos;
}

auto Lobby::UpdateFreeCamera(float deltaTime) -> void {
    if (!_camera) return;
    auto& input = *_app->Input();
    auto& camera = *_camera;

    float moveSpeed = 3.5f * deltaTime;
    if (input.GetKey(GLFW_KEY_LEFT_SHIFT) || input.GetKey(GLFW_KEY_RIGHT_SHIFT)) {
        moveSpeed *= 3.0f;
    }

    if (input.GetKey(GLFW_KEY_W)) { camera.Position += camera.GetFront() * moveSpeed; }
    if (input.GetKey(GLFW_KEY_S)) { camera.Position -= camera.GetFront() * moveSpeed; }
    if (input.GetKey(GLFW_KEY_D)) { camera.Position -= camera.GetRight() * moveSpeed; }
    if (input.GetKey(GLFW_KEY_A)) { camera.Position += camera.GetRight() * moveSpeed; }
    if (input.GetKey(GLFW_KEY_E)) { camera.Position += glm::vec3(0.0f, 1.0f, 0.0f) * moveSpeed; }
    if (input.GetKey(GLFW_KEY_Q)) { camera.Position -= glm::vec3(0.0f, 1.0f, 0.0f) * moveSpeed; }

    if (input.GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        input.SetMouseCapture(true);
        if (!input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            const glm::vec2 mouseDelta = input.GetMouseDelta();
            camera.Yaw -= mouseDelta.x * 0.1f;
            camera.Pitch -= mouseDelta.y * 0.1f;
        }
    } else {
        input.SetMouseCapture(false);
    }

    const float keyboardLookSpeed = 70.0f * deltaTime;
    if (input.GetKey(GLFW_KEY_LEFT))  { camera.Yaw += keyboardLookSpeed; }
    if (input.GetKey(GLFW_KEY_RIGHT)) { camera.Yaw -= keyboardLookSpeed; }
    if (input.GetKey(GLFW_KEY_UP))    { camera.Pitch += keyboardLookSpeed; }
    if (input.GetKey(GLFW_KEY_DOWN))  { camera.Pitch -= keyboardLookSpeed; }

    camera.Pitch = glm::clamp(camera.Pitch, -89.0f, 89.0f);
    camera.Update();
}

auto Lobby::CheckPortals() -> bool {
    if (!_camera || _portals.empty()) return false;

    const glm::vec3 pos = _camera->Position;
    bool insideAny = false;
    for (const auto& portal : _portals) {
        const glm::vec2 flat{pos.x - portal.Position.x, pos.z - portal.Position.z};
        if (glm::length(flat) <= portal.Radius) {
            insideAny = true;
            // Only trigger once we've stepped off the spawn portal at least once.
            if (!_portalCooldown && _app->Lobbies()->Has(portal.TargetLobby)) {
                _app->Lobbies()->GoTo(portal.TargetLobby);
                return true;
            }
        }
    }
    // Re-arm portals once the player has walked clear of all of them.
    if (!insideAny) {
        _portalCooldown = false;
    }
    return false;
}
