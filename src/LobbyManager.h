#pragma once

#include <memory>
#include <string>
#include <vector>

class App;
class Lobby;
class BeSceneManager;

// Wraps be-engine's BeSceneManager to add lobby navigation semantics:
//   - walk INTO a portal  -> GoTo(target), remembering where you came from
//   - walk OUT / go back   -> GoBack() returns you to the previous lobby
//   - the previous lobby is remembered across the session (history stack) AND
//     across runs (last-visited lobby persisted to a small state file).
//
// The actual crash-safe scene swap is delegated to BeSceneManager
// (RequestSceneChange + ApplyPendingSceneChange at end of frame).
class LobbyManager {
public:
    explicit LobbyManager(App* app);
    ~LobbyManager();

    // Register a lobby. Ownership transfers to the underlying scene manager.
    auto Add(std::unique_ptr<Lobby> lobby) -> void;

    // Prepare() every registered lobby (one-time heavy setup).
    auto PrepareAll() -> void;

    // Enter a lobby for the first frame. Uses the persisted last-visited lobby if
    // `preferPersisted` and one exists; otherwise `fallback`.
    auto Start(const std::string& fallback, bool preferPersisted = true) -> void;

    // Walk into a portal: switch to `target`, pushing the current lobby onto the
    // history stack so GoBack() returns here. No-op if already transitioning.
    auto GoTo(const std::string& target) -> void;

    // Walk out / back: pop the history stack and return to the previous lobby.
    // If the stack is empty, does nothing.
    auto GoBack() -> bool;

    // Apply any pending switch (call once per frame, at end of frame).
    auto ApplyPending() -> void;

    auto Active() const -> Lobby*;
    auto Previous() const -> std::string;   // top of history stack, or "" if none
    auto CurrentName() const -> const std::string& { return _current; }
    auto Has(const std::string& name) const -> bool;

private:
    auto PerformSwitch(const std::string& target, const std::string& arrivalFrom) -> void;
    auto StateFilePath() const -> std::string;
    auto LoadPersisted() const -> std::string;
    auto Persist(const std::string& lobby) const -> void;

    App* _app = nullptr;
    std::unique_ptr<BeSceneManager> _scenes;
    std::vector<std::string> _registered;   // lobby names, in registration order

    std::string _current;                   // name of the active lobby
    std::vector<std::string> _history;       // stack of lobbies we can walk back to
    bool _switching = false;                 // a switch is pending this frame
    std::string _pendingTarget;
    std::string _pendingFrom;
};
