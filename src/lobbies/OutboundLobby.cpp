#include "OutboundLobby.h"

#include <algorithm>
#include <iostream>

#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BeMesh.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

#include "App.h"
#include "AssetManager.h"

namespace {
    // A 1x1 upright quad in the XY plane, front face toward +Z, UVs right-side-up.
    // Standing geometry with correct UVs avoids any rotate-a-floor-plane guesswork.
    auto UprightQuad() -> std::shared_ptr<BeMesh> {
        auto mesh = std::make_shared<BeMesh>();
        const glm::vec4 tan{1, 0, 0, 1}, col{1, 1, 1, 1};
        const glm::vec3 nF{0, 0, 1};
        // front face (+Z). This engine flips textures on load (top verts V=1) and the
        // front winding mirrors U, so left verts get U=1, right U=0. Verified visually.
        mesh->Vertices.push_back({.Position = {-0.5f,  0.5f, 0}, .Normal = nF, .Color = col, .UV0 = {1, 1}, .Tangent = tan});
        mesh->Vertices.push_back({.Position = { 0.5f,  0.5f, 0}, .Normal = nF, .Color = col, .UV0 = {0, 1}, .Tangent = tan});
        mesh->Vertices.push_back({.Position = { 0.5f, -0.5f, 0}, .Normal = nF, .Color = col, .UV0 = {0, 0}, .Tangent = tan});
        mesh->Vertices.push_back({.Position = {-0.5f, -0.5f, 0}, .Normal = nF, .Color = col, .UV0 = {1, 0}, .Tangent = tan});
        mesh->Indices = {0, 1, 2, 0, 2, 3};
        mesh->Slices.push_back({.IndexCount = 6, .StartIndexLocation = 0, .BaseVertexLocation = 0});
        return mesh;
    }
}

OutboundLobby::OutboundLobby(App* app) : PbrRoomLobby(app, "outbound") {
    _defaultSpawnPos = {0.0f, 1.8f, 7.0f};
    _defaultSpawnLook = {0.0f, 1.6f, 0.0f};
    // Portal to the memory palace, set off to the right so it and its sign are
    // clearly visible alongside the screenshot gallery (not hidden behind it).
    _portals.push_back({ .TargetLobby = "memory-palace", .Position = {7.0f, 0.0f, 1.0f}, .Radius = 1.4f, .Label = "Memory Palace" });
}

auto OutboundLobby::AddScreenshot(const std::filesystem::path& path) -> void {
    const std::string idx = std::to_string(_panels.size());
    auto texture = BeTexture::Create("outbound-shot-" + idx)
        .LoadFromFile(path)
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetMipsAuto()
        .GenerateMips()
        .AddToRegistry()
        .Build();
    if (!texture) {
        std::cerr << "zeen[outbound]: failed to load screenshot " << path << '\n';
        return;
    }

    auto mesh = UprightQuad();
    auto prop = BeProp::FromMesh(mesh, Shader(), "geometry-main");
    if (prop->Materials.empty()) return;
    prop->Slices[0].TwoSided = true;
    prop->Materials[0]->SetTexture("Diffuse_or_Albedo", texture);
    prop->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-clamp"));
    prop->Materials[0]->SetFloat3("BaseColor", glm::vec3(1.0f));
    prop->Materials[0]->SetFloat("Roughness", 0.85f);
    prop->Materials[0]->SetFloat("Metallic", 0.0f);
    prop->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.0f));
    _machine->RegisterMesh(mesh);

    const float aspect = texture->Height > 0
        ? static_cast<float>(texture->Width) / static_cast<float>(texture->Height) : 1.4f;
    const float height = 2.6f;   // big enough to read from spawn

    // Upright quad: X = width, Y = height.
    Panel p;
    p.prop = prop;
    p.scale = glm::vec3(height * aspect, height, 1.0f);
    p.name = "shot-" + idx;
    _panels.push_back(std::move(p));
}

auto OutboundLobby::BuildContent() -> void {
    const auto cardsDir = _app->Assets()->LobbyRoot("outbound") / "cards";
    std::vector<std::filesystem::path> images;
    if (std::filesystem::exists(cardsDir)) {
        for (const auto& e : std::filesystem::directory_iterator(cardsDir)) {
            if (e.is_regular_file() && AssetManager::IsSupportedAsset(e.path())) images.push_back(e.path());
        }
        std::ranges::sort(images);
    }
    for (const auto& img : images) AddScreenshot(img);

    // Position the panels in a centred row at eye height, a readable distance ahead.
    const float spacing = 4.2f;
    const float startX = _panels.empty() ? 0.0f
        : -spacing * 0.5f * static_cast<float>(_panels.size() - 1);
    for (size_t i = 0; i < _panels.size(); ++i) {
        _panels[i].position = glm::vec3(
            startX + spacing * static_cast<float>(i),
            1.7f,      // eye height
            0.0f);
    }
}

auto OutboundLobby::SubmitContent(float now) -> void {
    // The quad already stands upright facing +Z (the spawn) — no rotation needed.
    for (const auto& panel : _panels) {
        _machine->AddGeometry({
            .Name = panel.name,
            .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(panel.position, glm::quat(), panel.scale),
            .Prop = panel.prop,
        });
    }
}

auto OutboundLobby::SubmitLights() -> void {
    // Soft overhead sun + a bright key light in front of the gallery so the
    // screenshots read clearly.
    _machine->AddSunLight({
        .Direction = glm::normalize(glm::vec3(0.0f, -1.0f, -0.3f)),
        .Color = glm::vec3(1.0f), .Power = 3.0f, .CastsShadows = false,
        .ShadowViewProjection = glm::mat4(1.0f), .ShadowMapResolution = 1,
        .ShadowMap = BeAssetRegistry::GetTexture("black"),
    });
    _machine->AddPointLight({
        .Name = "gallery-key", .Position = glm::vec3(0.0f, 2.4f, 4.0f), .Radius = 14.0f,
        .Color = glm::vec3(1.0f, 0.99f, 0.97f), .Power = 3.5f, .CastsShadows = false,
        .ShadowMapResolution = 1, .ShadowNearPlane = 0.1f,
        .ShadowMap = BeAssetRegistry::GetTexture("black-cube"),
    });
}
