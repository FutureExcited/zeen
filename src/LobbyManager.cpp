#include "LobbyManager.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "scenes/BeSceneManager.h"

#include "App.h"
#include "AssetManager.h"
#include "Lobby.h"

LobbyManager::LobbyManager(App* app)
    : _app(app), _scenes(std::make_unique<BeSceneManager>()) {}

LobbyManager::~LobbyManager() = default;

auto LobbyManager::Add(std::unique_ptr<Lobby> lobby) -> void {
    _registered.push_back(lobby->Name());
    _scenes->RegisterScene(lobby->Name(), std::move(lobby));
}

auto LobbyManager::Has(const std::string& name) const -> bool {
    return std::find(_registered.begin(), _registered.end(), name) != _registered.end();
}

auto LobbyManager::PrepareAll() -> void {
    for (const auto& name : _registered) {
        if (auto* lobby = dynamic_cast<Lobby*>(_scenes->GetScene(name))) {
            lobby->Prepare();
        }
    }
}

auto LobbyManager::Start(const std::string& fallback, bool preferPersisted) -> void {
    std::string target = fallback;
    if (preferPersisted) {
        const std::string persisted = LoadPersisted();
        if (!persisted.empty() && Has(persisted)) {
            target = persisted;
        }
    }
    if (!Has(target) && !_registered.empty()) {
        target = _registered.front();
    }
    PerformSwitch(target, /*arrivalFrom=*/"");
    _scenes->ApplyPendingSceneChange();   // enter immediately on first frame
}

auto LobbyManager::GoTo(const std::string& target) -> void {
    if (_switching || target == _current || !Has(target)) return;
    if (!_current.empty()) {
        _history.push_back(_current);
    }
    _pendingTarget = target;
    _pendingFrom = _current;
    _switching = true;
}

auto LobbyManager::GoBack() -> bool {
    if (_switching || _history.empty()) return false;
    const std::string target = _history.back();
    _history.pop_back();
    _pendingTarget = target;
    _pendingFrom = _current;   // arriving "back" — spawn at the door we came in
    _switching = true;
    return true;
}

auto LobbyManager::ApplyPending() -> void {
    if (_switching) {
        PerformSwitch(_pendingTarget, _pendingFrom);
        _switching = false;
        _pendingTarget.clear();
        _pendingFrom.clear();
    }
    _scenes->ApplyPendingSceneChange();
}

auto LobbyManager::PerformSwitch(const std::string& target, const std::string& arrivalFrom) -> void {
    if (auto* lobby = dynamic_cast<Lobby*>(_scenes->GetScene(target))) {
        lobby->SetArrivalFrom(arrivalFrom);
    }
    _scenes->RequestSceneChange(target);
    _current = target;
    Persist(_current);

    // Re-point the upload handler at whatever lobby is now active.
    if (_app->Assets()) {
        _app->Assets()->ClearDropHandler();
    }
}

auto LobbyManager::Active() const -> Lobby* {
    return dynamic_cast<Lobby*>(_scenes->GetActiveScene());
}

auto LobbyManager::Previous() const -> std::string {
    return _history.empty() ? std::string{} : _history.back();
}

auto LobbyManager::StateFilePath() const -> std::string {
    return (_app->Assets()->UploadsDir().parent_path() / "last-lobby.txt").string();
}

auto LobbyManager::LoadPersisted() const -> std::string {
    std::ifstream in(StateFilePath());
    if (!in) return {};
    std::string name;
    std::getline(in, name);
    // trim trailing whitespace/newline
    while (!name.empty() && (name.back() == '\n' || name.back() == '\r' || name.back() == ' ')) {
        name.pop_back();
    }
    return name;
}

auto LobbyManager::Persist(const std::string& lobby) const -> void {
    std::ofstream out(StateFilePath(), std::ios::trunc);
    if (out) out << lobby << '\n';
}
