#include "App.h"

#include <filesystem>

#if defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h>
#endif

#include <umbrellas/include-glfw.h>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeWindow.h"

#include "AssetManager.h"
#include "LobbyManager.h"
#include "lobbies/ZoroLobby.h"
#include "lobbies/MemoryPalaceLobby.h"
#include "lobbies/OutboundLobby.h"

namespace {
    auto MoveWorkingDirectoryToExecutableDir() -> void {
#if defined(__APPLE__)
        char buffer[PATH_MAX];
        uint32_t size = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &size) == 0) {
            std::filesystem::current_path(
                std::filesystem::weakly_canonical(buffer).parent_path());
        }
#endif
    }
}

App::App() = default;
App::~App() = default;

auto App::Run() -> int {
    MoveWorkingDirectoryToExecutableDir();

    _window = std::make_shared<BeWindow>(1280, 720, "zeen", BeWindowMode::Windowed);
    _width = static_cast<uint32_t>(_window->GetReportedPixelWidth());
    _height = static_cast<uint32_t>(_window->GetReportedPixelHeight());

    _renderer = std::make_shared<BeRenderer>(_width, _height,
        static_cast<void*>(_window->GetGlfwWindow()));
    _renderer->LaunchDevice();
    _width = _renderer->GetSwapchainPixelWidth();
    _height = _renderer->GetSwapchainPixelHeight();

    _input = std::make_unique<BeInput>(_window->GetGlfwWindow());

    BeAssetRegistry::InjectRenderer(_renderer);
    RegisterDefaultTextures();

    _assets = std::make_unique<AssetManager>(this);
    _assets->InstallDropHandler(_window->GetGlfwWindow());

    _lobbies = std::make_unique<LobbyManager>(this);
    SetupLobbies();

    MainLoop();

    BeAssetRegistry::Shutdown();
    return 0;
}

auto App::RegisterDefaultTextures() -> void {
    BeTexture::Create("white")
        .SetSize(1, 1).SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm).FillWithColor(glm::vec4(1.0f))
        .AddToRegistry().BuildNoReturn();
    BeTexture::Create("black")
        .SetSize(1, 1).SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm).FillWithColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
        .AddToRegistry().BuildNoReturn();
    BeTexture::Create("black-cube")
        .SetSize(1, 1).SetCubemap(true).SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm).FillWithColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
        .AddToRegistry().BuildNoReturn();
    BeTexture::Create("default-orm")
        .SetSize(1, 1).SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm).FillWithColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f))
        .AddToRegistry().BuildNoReturn();
    BeTexture::Create("flat-normal")
        .SetSize(1, 1).SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm).FillWithColor(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f))
        .AddToRegistry().BuildNoReturn();
}

auto App::SetupLobbies() -> void {
    _lobbies->Add(std::make_unique<ZoroLobby>(this));
    _lobbies->Add(std::make_unique<MemoryPalaceLobby>(this));
    _lobbies->Add(std::make_unique<OutboundLobby>(this));

    _lobbies->PrepareAll();
    // Reopen the lobby you were last in; first run defaults to zoro.
    _lobbies->Start("zoro", /*preferPersisted=*/true);
}

auto App::MainLoop() -> void {
    double lastTime = glfwGetTime();
    while (!_window->ShouldClose()) {
        _window->PollEvents();
        _input->Update();

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        if (_input->GetKeyDown(GLFW_KEY_ESCAPE)) {
            _window->RequestClose();
        }

        if (auto* active = _lobbies->Active()) {
            active->Tick(dt);
        }

        _renderer->Render();
        _lobbies->ApplyPending();
    }
}
