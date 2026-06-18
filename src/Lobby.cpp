#include "Lobby.h"

#include <umbrellas/include-glfw.h>

#include "BeCamera.h"
#include "BeInput.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

#include "App.h"
#include "LobbyHud.h"
#include "LobbyManager.h"

Lobby::~Lobby() = default;

auto Lobby::CameraPosition() const -> glm::vec3 {
    return _camera ? _camera->Position : glm::vec3(0.0f);
}

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
    // Re-register the shared nav HUD on top of this lobby's passes (OnEnter did
    // ClearPasses, which dropped it from the renderer's list).
    if (_app->Hud()) _app->Hud()->RegisterWithRenderer();
    _arrivalFrom.clear();
}

auto Lobby::SpawnAtReturnPortalFrom(const std::string& fromLobby) -> void {
    if (!_camera) return;
    for (const auto& portal : _portals) {
        if (portal.TargetLobby == fromLobby) {
            // Spawn a step toward the room centre from the portal, facing inward —
            // as if you just stepped through that door back into this room.
            glm::vec3 inward = _defaultSpawnLook - portal.Position;
            inward.y = 0.0f;
            if (glm::length(inward) < 0.001f) inward = glm::vec3(0.0f, 0.0f, 1.0f);
            inward = glm::normalize(inward);

            _camera->Position = portal.Position + inward * 2.0f;   // in front of the portal
            _camera->Position.y = _defaultSpawnPos.y;

            // Face into the room.
            _camera->Yaw = glm::degrees(glm::atan(inward.z, inward.x));
            _camera->Pitch = 0.0f;
            _camera->Update();
            return;
        }
    }
    _camera->Position = _defaultSpawnPos;
}

namespace {
    auto HorizontalDirection(glm::vec3 direction, glm::vec3 fallback) -> glm::vec3 {
        direction.y = 0.0f;
        const float lenSq = glm::dot(direction, direction);
        if (lenSq < 0.0001f) {
            fallback.y = 0.0f;
            return glm::normalize(fallback);
        }
        return direction / glm::sqrt(lenSq);
    }

    auto ApplyFriction(glm::vec3 velocity, float friction, float stopSpeed, float deltaTime) -> glm::vec3 {
        const float speed = glm::length(velocity);
        if (speed < 0.001f) return {0.0f, 0.0f, 0.0f};
        const float control = glm::max(speed, stopSpeed);
        const float drop = control * friction * deltaTime;
        const float newSpeed = glm::max(speed - drop, 0.0f);
        return velocity * (newSpeed / speed);
    }

    auto Accelerate(glm::vec3 velocity, glm::vec3 wishDir, float wishSpeed, float acceleration, float deltaTime) -> glm::vec3 {
        if (wishSpeed <= 0.0f) return velocity;
        const float currentSpeed = glm::dot(velocity, wishDir);
        const float addSpeed = wishSpeed - currentSpeed;
        if (addSpeed <= 0.0f) return velocity;
        const float accelSpeed = glm::min(acceleration * wishSpeed * deltaTime, addSpeed);
        return velocity + wishDir * accelSpeed;
    }
}

// Full Quake-style player movement: walk/jump/bunnyhop, double-tap-space to toggle
// fly, crouch (shift), gravity, ground accel + friction, air control. Ported from
// the working memory-palace; now shared by every lobby.
auto Lobby::UpdateFreeCamera(float deltaTime) -> void {
    if (!_camera) return;
    auto& input = *_app->Input();
    auto& camera = *_camera;
    auto& controller = _controller;

    constexpr float standingEyeHeight = 1.65f;
    constexpr float crouchEyeHeight = 1.45f;
    constexpr float walkSpeed = 5.8f;
    constexpr float flySpeed = 6.4f;
    constexpr float climbSpeed = 4.0f;
    constexpr float descendSpeed = 2.6f;
    constexpr float jumpVelocity = 4.15f;
    constexpr float gravity = 15.5f;
    constexpr float groundAcceleration = 13.0f;
    constexpr float airAcceleration = 5.6f;
    constexpr float groundFriction = 7.0f;
    constexpr float stopSpeed = 2.0f;
    constexpr float maxBhopSpeed = 13.0f;
    constexpr double doubleTapSeconds = 0.30;

    const bool sprintHeld = input.GetKey(GLFW_KEY_LEFT_CONTROL) || input.GetKey(GLFW_KEY_RIGHT_CONTROL);
    float moveSpeed = (controller.Flying ? flySpeed : walkSpeed) * deltaTime;
    if (sprintHeld) moveSpeed *= 2.4f;

    const glm::vec3 forward = HorizontalDirection(camera.GetFront(), {0.0f, 0.0f, 1.0f});
    const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));

    glm::vec3 movement(0.0f);
    if (input.GetKey(GLFW_KEY_W)) movement += forward;
    if (input.GetKey(GLFW_KEY_S)) movement -= forward;
    if (input.GetKey(GLFW_KEY_D)) movement += right;
    if (input.GetKey(GLFW_KEY_A)) movement -= right;

    const float movementLenSq = glm::dot(movement, movement);
    const glm::vec3 wishDir = movementLenSq > 0.0001f ? movement / glm::sqrt(movementLenSq) : glm::vec3(0.0f);

    if (input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) || input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        input.SetMouseCapture(true);
    }
    if (input.IsMouseCaptured()) {
        const glm::vec2 mouseDelta = input.GetMouseDelta();
        camera.Yaw -= mouseDelta.x * 0.1f;
        camera.Pitch -= mouseDelta.y * 0.1f;
    } else {
        input.SetMouseCapture(false);
    }

    const float keyboardLookSpeed = 70.0f * deltaTime;
    if (input.GetKey(GLFW_KEY_LEFT))  camera.Yaw += keyboardLookSpeed;
    if (input.GetKey(GLFW_KEY_RIGHT)) camera.Yaw -= keyboardLookSpeed;
    if (input.GetKey(GLFW_KEY_UP))    camera.Pitch += keyboardLookSpeed;
    if (input.GetKey(GLFW_KEY_DOWN))  camera.Pitch -= keyboardLookSpeed;

    const glm::vec2 scroll = input.GetScrollDelta();
    if (scroll.y != 0.0f) camera.Fov = glm::clamp(camera.Fov - scroll.y, 25.0f, 85.0f);

    const bool shiftHeld = input.GetKey(GLFW_KEY_LEFT_SHIFT) || input.GetKey(GLFW_KEY_RIGHT_SHIFT);
    const bool spaceHeld = input.GetKey(GLFW_KEY_SPACE);
    const bool spaceDown = input.GetKeyDown(GLFW_KEY_SPACE);
    const bool spaceUp = input.GetKeyUp(GLFW_KEY_SPACE);
    const double now = glfwGetTime();
    bool toggledFlightThisFrame = false;

    if (spaceUp) {
        controller.SpaceReleasedAfterTap = true;
        controller.SuppressSpaceClimbUntilReleased = false;
    }
    if (controller.WaitingForFlightTap && (now - controller.LastSpaceTapTime) > doubleTapSeconds) {
        controller.WaitingForFlightTap = false;
    }
    if (spaceDown) {
        const bool doubleTapped =
            controller.WaitingForFlightTap && controller.SpaceReleasedAfterTap &&
            (now - controller.LastSpaceTapTime) <= doubleTapSeconds;
        if (doubleTapped) {
            controller.Flying = !controller.Flying;
            controller.Grounded = false;
            controller.VerticalVelocity = 0.0f;
            controller.EyeHeight = standingEyeHeight;
            controller.LastSpaceTapTime = -100.0;
            controller.WaitingForFlightTap = false;
            controller.SpaceReleasedAfterTap = false;
            controller.SuppressSpaceClimbUntilReleased = controller.Flying;
            toggledFlightThisFrame = true;
        } else {
            controller.WaitingForFlightTap = true;
            controller.LastSpaceTapTime = now;
            controller.SpaceReleasedAfterTap = false;
            if (!controller.Flying && controller.Grounded) {
                controller.VerticalVelocity = jumpVelocity;
                controller.Grounded = false;
            }
        }
    }

    if (controller.Flying) {
        if (movementLenSq > 0.0001f) camera.Position += wishDir * moveSpeed;
        controller.HorizontalVelocity = glm::vec3(0.0f);
        if (spaceHeld && !toggledFlightThisFrame && !controller.SuppressSpaceClimbUntilReleased) {
            camera.Position.y += climbSpeed * deltaTime;
        }
        if (shiftHeld) camera.Position.y -= descendSpeed * deltaTime;
        camera.Position.y = glm::max(camera.Position.y, crouchEyeHeight);
    } else {
        const float targetEyeHeight = shiftHeld ? crouchEyeHeight : standingEyeHeight;
        const bool wantsJump = spaceHeld && !toggledFlightThisFrame;
        const float wishSpeed = (sprintHeld ? walkSpeed * 1.35f : walkSpeed) * (shiftHeld ? 0.66f : 1.0f);

        if (controller.Grounded) {
            if (!wantsJump) {
                controller.HorizontalVelocity = ApplyFriction(controller.HorizontalVelocity, groundFriction, stopSpeed, deltaTime);
            }
            controller.HorizontalVelocity = Accelerate(controller.HorizontalVelocity, wishDir,
                movementLenSq > 0.0001f ? wishSpeed : 0.0f, groundAcceleration, deltaTime);
            controller.EyeHeight = glm::mix(controller.EyeHeight, targetEyeHeight, glm::clamp(deltaTime * 10.0f, 0.0f, 1.0f));
            camera.Position.y = controller.EyeHeight;
            if (wantsJump) {
                controller.VerticalVelocity = jumpVelocity;
                controller.Grounded = false;
                controller.EyeHeight = standingEyeHeight;
                camera.Position.y = standingEyeHeight + 0.02f;
            }
        } else {
            controller.HorizontalVelocity = Accelerate(controller.HorizontalVelocity, wishDir,
                movementLenSq > 0.0001f ? walkSpeed : 0.0f, airAcceleration, deltaTime);
            const float airSpeed = glm::length(controller.HorizontalVelocity);
            if (airSpeed > maxBhopSpeed) controller.HorizontalVelocity *= maxBhopSpeed / airSpeed;
            controller.VerticalVelocity -= gravity * deltaTime;
            camera.Position.y += controller.VerticalVelocity * deltaTime;
            if (camera.Position.y <= targetEyeHeight) {
                camera.Position.y = targetEyeHeight;
                controller.EyeHeight = targetEyeHeight;
                controller.VerticalVelocity = 0.0f;
                controller.Grounded = true;
            }
        }
        camera.Position += controller.HorizontalVelocity * deltaTime;
    }

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
