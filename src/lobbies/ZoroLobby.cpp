#include "ZoroLobby.h"

#include <filesystem>
#include <iostream>

#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BeMesh.h"
#include "BeProp.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

#include "App.h"
#include "AssetManager.h"

namespace {
    auto NormalizeMeshToHeight(const std::shared_ptr<BeMesh>& mesh, float targetHeight) -> void {
        if (!mesh || mesh->Vertices.empty()) return;
        glm::vec3 mn = mesh->Vertices.front().Position;
        glm::vec3 mx = mn;
        for (const auto& v : mesh->Vertices) { mn = glm::min(mn, v.Position); mx = glm::max(mx, v.Position); }
        const auto size = mx - mn;
        if (size.y <= 0.0001f) return;
        const float scale = targetHeight / size.y;
        const glm::vec3 base{(mn.x + mx.x) * 0.5f, mn.y, (mn.z + mx.z) * 0.5f};
        for (auto& v : mesh->Vertices) v.Position = (v.Position - base) * scale;
    }

    auto FindModel(const std::filesystem::path& root) -> std::filesystem::path {
        const std::filesystem::path candidates[] = {
            root / "zoro/scene.gltf", root / "zoro/scene.glb",
            root / "zoro/model.gltf", root / "zoro/model.glb",
            root / "zoro/ZORO(finished).obj",
        };
        for (const auto& c : candidates) if (std::filesystem::exists(c)) return c;
        return {};
    }
}

ZoroLobby::ZoroLobby(App* app) : PbrRoomLobby(app, "zoro") {
    _defaultSpawnPos = {0.0f, 1.8f, -5.0f};
    _defaultSpawnLook = {0.0f, 1.35f, 0.0f};
    _portals.push_back({ .TargetLobby = "memory-palace", .Position = {6.0f, 0.0f, 0.0f},  .Radius = 1.3f, .Label = "Memory Palace" });
    _portals.push_back({ .TargetLobby = "outbound",      .Position = {-6.0f, 0.0f, 0.0f}, .Radius = 1.3f, .Label = "Outbound" });
}

auto ZoroLobby::BuildContent() -> void {
    const auto root = _app->Assets()->LobbyRoot("zoro");
    const auto modelPath = FindModel(root);
    if (modelPath.empty()) {
        std::cerr << "zeen[zoro]: model not found under " << (root / "zoro") << "; empty room.\n";
        return;
    }
    _zoro = _machine->LoadProp(modelPath, Shader());
    NormalizeMeshToHeight(_zoro->Mesh, 3.0f);
    for (const auto& m : _zoro->Materials) {
        m->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-clamp"));
        m->SetFloat("Roughness", 0.62f);
    }
    _hasModel = true;
}

auto ZoroLobby::SubmitContent(float now) -> void {
    if (!_hasModel) return;
    _machine->AddGeometry({
        .Name = "zoro",
        .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix({0.0f, 0.0f, 0.0f},
            glm::quat(glm::vec3(0.0f, glm::radians(180.0f), 0.0f)), glm::vec3(1.0f)),
        .Prop = _zoro,
    });
}
