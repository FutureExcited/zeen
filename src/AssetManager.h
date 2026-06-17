#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

class App;
struct GLFWwindow;

// Centralised asset management for zeen.
//
// Asset layout:
//   <app>/assets/                 - shared assets shipped with zeen (shaders etc.)
//   <app>/assets/lobbies/<name>/  - per-lobby assets
//   <user-data>/zeen/uploads/     - files a user drops into a lobby at runtime
//
// Runtime uploads: a user walking in a lobby can drag-and-drop image/model files
// onto the window. AssetManager copies them into the persistent uploads dir and
// notifies the active lobby, which spawns them as objects in the world.
class AssetManager {
public:
    explicit AssetManager(App* app);

    // Locate the shared assets root (handles app-bundle vs run-from-source).
    auto SharedRoot() const -> const std::filesystem::path& { return _sharedRoot; }
    auto LobbyRoot(const std::string& lobbyName) const -> std::filesystem::path;

    // Persistent directory for user-uploaded files (created on first use).
    auto UploadsDir() const -> const std::filesystem::path& { return _uploadsDir; }

    // Install the GLFW drop callback on the window. Dropped files are copied into
    // UploadsDir and forwarded to the registered drop handler.
    auto InstallDropHandler(GLFWwindow* window) -> void;

    // The active lobby registers here to receive dropped/uploaded files.
    // The handler receives the persistent path of each accepted upload.
    using DropHandler = std::function<void(const std::filesystem::path&)>;
    auto SetDropHandler(DropHandler handler) -> void { _dropHandler = std::move(handler); }
    auto ClearDropHandler() -> void { _dropHandler = nullptr; }

    // Test seam: invoke the active drop handler with an already-stored path.
    auto InvokeDropHandlerForTest(const std::filesystem::path& stored) -> void {
        if (_dropHandler) _dropHandler(stored);
    }

    // Copy a single dropped file into UploadsDir; returns the stored path (or empty
    // if the extension isn't a supported asset type).
    auto IngestUpload(const std::filesystem::path& source) const -> std::filesystem::path;

    static auto IsSupportedAsset(const std::filesystem::path& path) -> bool;

private:
    auto ResolveSharedRoot() const -> std::filesystem::path;
    auto ResolveUploadsDir() const -> std::filesystem::path;
    auto OnFilesDropped(const std::vector<std::filesystem::path>& paths) -> void;

    static void GlfwDropCallback(GLFWwindow* window, int count, const char** paths);

    App* _app = nullptr;
    std::filesystem::path _sharedRoot;
    std::filesystem::path _uploadsDir;
    DropHandler _dropHandler;
};
