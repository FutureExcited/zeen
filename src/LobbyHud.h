#pragma once

#include <memory>
#include <string>
#include <vector>

class App;
class BeImGuiPass;

// On-screen navigation HUD drawn over every lobby. Shows which lobby you're in,
// lists the portals (where each leads + how far), and gives clickable buttons to
// jump straight to any lobby. Backed by the engine's BeImGuiPass.
//
// ONE HUD, owned by App and created once (the ImGui context is global, so it must
// not be created per-lobby). Each lobby re-registers it with the renderer in
// OnEnter (after ClearPasses) so the overlay survives lobby switches. The Draw
// callback reads whatever lobby is currently active.
class LobbyHud {
public:
    explicit LobbyHud(App* app);
    ~LobbyHud();

    auto Initialise() -> void;            // create the ImGui pass once
    auto RegisterWithRenderer() -> void;  // re-add to the renderer's pass list (call in OnEnter)

private:
    auto Draw() -> void;   // the per-frame ImGui callback

    App* _app = nullptr;
    std::unique_ptr<BeImGuiPass> _imguiPass;
};
