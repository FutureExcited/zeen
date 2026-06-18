#include "App.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

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
    // Bring the window to the front on launch (GLFW windows otherwise open behind).
    glfwShowWindow(_window->GetGlfwWindow());
    glfwFocusWindow(_window->GetGlfwWindow());
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

    if (const char* st = std::getenv("ZEEN_SELFTEST"); st && *st) {
        const int rc = SelfTest();
        BeAssetRegistry::Shutdown();
        return rc;
    }

    MainLoop();

    BeAssetRegistry::Shutdown();
    return 0;
}

// Drives the REAL lobby manager + asset systems through a lobby round-trip and an
// upload, rendering a frame between steps so passes actually build. Used to verify
// the headless behaviours that can't be exercised by hand from a shell.
auto App::SelfTest() -> int {
    // Mirror MainLoop exactly: tick the active lobby (which submits geometry + lights)
    // BEFORE Render(), or the lighting pass reads an empty sun-light list and crashes.
    auto frame = [&] {
        _window->PollEvents();
        _input->Update();
        if (auto* active = _lobbies->Active()) active->Tick(0.016f);
        _renderer->Render();
        _lobbies->ApplyPending();
    };
    int failures = 0;
    auto check = [&](bool ok, const char* what) {
        std::cout << "[selftest] " << (ok ? "PASS" : "FAIL") << " — " << what << '\n';
        if (!ok) ++failures;
    };

    frame();   // settle into the starting lobby
    check(_lobbies->CurrentName() == "outbound", "starts in outbound");

    _lobbies->GoTo("memory-palace");
    frame();
    check(_lobbies->CurrentName() == "memory-palace", "GoTo(memory-palace) switched lobby");
    check(_lobbies->Previous() == "outbound", "history remembers outbound as previous");

    const bool wentBack = _lobbies->GoBack();
    frame();
    check(wentBack && _lobbies->CurrentName() == "outbound", "GoBack() returned to outbound");

    // Real upload through the AssetManager + active lobby's drop handler.
    if (const char* img = std::getenv("ZEEN_SELFTEST_IMAGE"); img && *img) {
        const auto stored = _assets->IngestUpload(img);
        check(!stored.empty() && std::filesystem::exists(stored), "IngestUpload stored the image");
        _assets->InvokeDropHandlerForTest(stored);   // routes to active lobby's spawner
        frame();
        check(true, "drop handler ran without crashing (object spawned)");
    }

    std::cout << "[selftest] " << (failures == 0 ? "ALL PASS" : "FAILURES") << " (" << failures << ")\n";
    return failures == 0 ? 0 : 2;
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
    _lobbies->Add(std::make_unique<OutboundLobby>(this));
    _lobbies->Add(std::make_unique<MemoryPalaceLobby>(this));

    _lobbies->PrepareAll();
    // Reopen the lobby you were last in; first run defaults to outbound (the hub).
    _lobbies->Start("outbound", /*preferPersisted=*/true);
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
