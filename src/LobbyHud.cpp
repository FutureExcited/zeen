#include "LobbyHud.h"

#include <algorithm>
#include <cmath>

#include <umbrellas/include-glm.h>

#include "imgui/BeImGuiPass.h"
#include "imgui/imgui.h"

#include "BeRenderer.h"

#include "App.h"
#include "Lobby.h"
#include "LobbyManager.h"

LobbyHud::LobbyHud(App* app) : _app(app) {}
LobbyHud::~LobbyHud() = default;

auto LobbyHud::Initialise() -> void {
    _imguiPass = std::make_unique<BeImGuiPass>(
        std::shared_ptr<BeWindow>(_app->Window(), [](BeWindow*) {}));   // non-owning
    // AddRenderPass injects the renderer into the pass; Initialise() needs it
    // (it reads the swapchain format), so register BEFORE initialising.
    _app->Renderer()->AddRenderPass(_imguiPass.get());
    _imguiPass->SetUICallback([this]() { Draw(); });
    _imguiPass->Initialise();
}

auto LobbyHud::RegisterWithRenderer() -> void {
    if (_imguiPass) _app->Renderer()->AddRenderPass(_imguiPass.get());
}

auto LobbyHud::Draw() -> void {
    auto* lobbies = _app->Lobbies();
    const auto& current = lobbies->CurrentName();

    // ── Top-left: where you are + navigation ──────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(24, 24), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.07f, 0.78f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.95f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.30f, 0.55f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.45f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.55f, 1.0f, 1.0f));
    ImGui::Begin("zeen-nav", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SetWindowFontScale(1.7f);
    ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "%s", LobbyManager::DisplayName(current).c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.72f, 1.0f), "GO TO");
    for (const auto& id : lobbies->Names()) {
        if (id == current) continue;
        const std::string label = "  " + LobbyManager::DisplayName(id) + "  ##go-" + id;
        if (ImGui::Button(label.c_str(), ImVec2(-1, 0))) {
            lobbies->GoTo(id);
        }
    }

    // Walk-back button if there's history.
    const std::string prev = lobbies->Previous();
    if (!prev.empty()) {
        ImGui::Spacing();
        const std::string back = "< Back to " + LobbyManager::DisplayName(prev) + "  [Backspace]##back";
        if (ImGui::Button(back.c_str(), ImVec2(-1, 0))) {
            lobbies->GoBack();
        }
    }

    // ── Portal distances (so walking is navigable too) ────────────────────────
    Lobby* activeLobby = lobbies->Active();
    if (activeLobby && !activeLobby->Portals().empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.72f, 1.0f), "PORTALS NEARBY");
        const glm::vec3 cam = activeLobby->CameraPosition();
        for (const auto& p : activeLobby->Portals()) {
            const glm::vec2 flat{cam.x - p.Position.x, cam.z - p.Position.z};
            const float dist = std::sqrt(flat.x * flat.x + flat.y * flat.y);
            const char* arrow = dist <= p.Radius + 0.3f ? ">> " : "-> ";
            ImGui::Text("%s%s  (%.0fm)", arrow,
                (p.Label.empty() ? LobbyManager::DisplayName(p.TargetLobby) : p.Label).c_str(), dist);
        }
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.62f, 1.0f),
        "WASD move - Space jump (x2 fly)\nClick to look - Esc free cursor - drop a file to add");

    ImGui::End();
    ImGui::PopStyleColor(5);
}
