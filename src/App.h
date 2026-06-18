#pragma once

#include <cstdint>
#include <memory>
#include <string>

class BeWindow;
class BeRenderer;
class BeInput;
class LobbyManager;
class AssetManager;

// The zeen host. Owns the window/renderer/input and the lobby manager, and runs
// the main loop. Lobbies (BeScene subclasses) are registered with the manager and
// share this App so they all render into the same window without re-launching.
class App {
public:
    App();
    ~App();

    auto Run() -> int;

    // Accessors for lobbies.
    auto Window() const -> BeWindow* { return _window.get(); }
    auto Renderer() const -> const std::shared_ptr<BeRenderer>& { return _renderer; }
    auto Input() const -> BeInput* { return _input.get(); }
    auto Lobbies() const -> LobbyManager* { return _lobbies.get(); }
    auto Assets() const -> AssetManager* { return _assets.get(); }

    auto Width() const -> uint32_t { return _width; }
    auto Height() const -> uint32_t { return _height; }

private:
    auto RegisterDefaultTextures() -> void;
    auto SetupLobbies() -> void;
    auto MainLoop() -> void;
    auto SelfTest() -> int;

    uint32_t _width = 0;
    uint32_t _height = 0;

    std::shared_ptr<BeWindow>   _window;
    std::shared_ptr<BeRenderer> _renderer;
    std::unique_ptr<BeInput>    _input;
    std::unique_ptr<LobbyManager> _lobbies;
    std::unique_ptr<AssetManager> _assets;
};
