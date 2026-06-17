#include "AssetManager.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

#if defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h>
#endif

#include <umbrellas/include-glfw.h>

#include "App.h"

namespace {
    // BeInput owns the GLFW window user pointer, so we route drop callbacks via a
    // small side table instead of stomping it.
    std::unordered_map<GLFWwindow*, AssetManager*>& DropRegistry() {
        static std::unordered_map<GLFWwindow*, AssetManager*> registry;
        return registry;
    }

    auto ExecutableDir() -> std::filesystem::path {
#if defined(__APPLE__)
        char buffer[PATH_MAX];
        uint32_t size = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &size) == 0) {
            return std::filesystem::weakly_canonical(buffer).parent_path();
        }
#endif
        return std::filesystem::current_path();
    }
}

AssetManager::AssetManager(App* app) : _app(app) {
    _sharedRoot = ResolveSharedRoot();
    _uploadsDir = ResolveUploadsDir();
    std::error_code ec;
    std::filesystem::create_directories(_uploadsDir, ec);
}

auto AssetManager::ResolveSharedRoot() const -> std::filesystem::path {
    // assets ship next to the executable (copied in by CMake), but also support
    // running straight from the source tree.
    const std::filesystem::path candidates[] = {
        ExecutableDir() / "assets",
        std::filesystem::current_path() / "assets",
        std::filesystem::current_path() / "src" / "assets",
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) {
            return std::filesystem::weakly_canonical(c);
        }
    }
    // Fall back to exe-dir/assets even if missing; lobbies will report a clear error.
    return ExecutableDir() / "assets";
}

auto AssetManager::ResolveUploadsDir() const -> std::filesystem::path {
    // Persistent, user-writable location: ~/.local/share/zeen/uploads (or $XDG / HOME).
    std::filesystem::path base;
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) {
        base = xdg;
    } else if (const char* home = std::getenv("HOME"); home && *home) {
        base = std::filesystem::path(home) / ".local" / "share";
    } else {
        base = std::filesystem::temp_directory_path();
    }
    return base / "zeen" / "uploads";
}

auto AssetManager::LobbyRoot(const std::string& lobbyName) const -> std::filesystem::path {
    return _sharedRoot / "lobbies" / lobbyName;
}

auto AssetManager::IsSupportedAsset(const std::filesystem::path& path) -> bool {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    static const char* kSupported[] = {
        ".png", ".jpg", ".jpeg", ".bmp", ".tga",      // images
        ".gltf", ".glb", ".obj", ".fbx",              // models
    };
    for (const char* s : kSupported) {
        if (ext == s) return true;
    }
    return false;
}

auto AssetManager::IngestUpload(const std::filesystem::path& source) const -> std::filesystem::path {
    if (!IsSupportedAsset(source) || !std::filesystem::exists(source)) {
        return {};
    }
    const std::filesystem::path dest = _uploadsDir / source.filename();
    std::error_code ec;
    std::filesystem::copy_file(source, dest,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "zeen: failed to ingest upload " << source << ": " << ec.message() << '\n';
        return {};
    }
    return dest;
}

auto AssetManager::InstallDropHandler(GLFWwindow* window) -> void {
    DropRegistry()[window] = this;
    glfwSetDropCallback(window, &AssetManager::GlfwDropCallback);
}

void AssetManager::GlfwDropCallback(GLFWwindow* window, int count, const char** paths) {
    auto it = DropRegistry().find(window);
    if (it == DropRegistry().end()) return;
    std::vector<std::filesystem::path> dropped;
    dropped.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        dropped.emplace_back(paths[i]);
    }
    it->second->OnFilesDropped(dropped);
}

auto AssetManager::OnFilesDropped(const std::vector<std::filesystem::path>& paths) -> void {
    for (const auto& p : paths) {
        const std::filesystem::path stored = IngestUpload(p);
        if (stored.empty()) {
            std::cerr << "zeen: ignored unsupported drop " << p << '\n';
            continue;
        }
        std::cout << "zeen: uploaded " << stored.filename() << '\n';
        if (_dropHandler) {
            _dropHandler(stored);
        }
    }
}
