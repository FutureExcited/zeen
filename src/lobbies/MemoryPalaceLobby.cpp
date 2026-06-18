#include "MemoryPalaceLobby.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>

#include <umbrellas/include-glfw.h>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMesh.h"
#include "BeMeshPrimitives.h"
#include "BePass.h"
#include "BePipelineBuilder.h"
#include "BeProp.h"
#include "BeRenderPass.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "sen-rhi/SenBackend.h"
#include "sen-rhi/SenShaderCompiler.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

#include "App.h"
#include "AssetManager.h"
#include "LobbyManager.h"

namespace {
    auto SafeTextureName(const std::filesystem::path& path, size_t index) -> std::string {
        auto stem = path.stem().string();
        for (auto& c : stem) {
            const bool ok = std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
            if (!ok) c = '_';
        }
        return "MemoryCard_" + std::to_string(index) + "_" + stem;
    }

    auto AddCardQuad(BeMesh& mesh,
        const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
        const glm::vec3& normal,
        const glm::vec2& uvA, const glm::vec2& uvB, const glm::vec2& uvC, const glm::vec2& uvD) -> void {
        const auto base = static_cast<uint32_t>(mesh.Vertices.size());
        const glm::vec4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
        const glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        const glm::vec3 n = glm::normalize(normal);
        mesh.Vertices.push_back(BeFullVertex{.Position = a, .Normal = n, .Color = color, .UV0 = uvA, .Tangent = tangent});
        mesh.Vertices.push_back(BeFullVertex{.Position = b, .Normal = n, .Color = color, .UV0 = uvB, .Tangent = tangent});
        mesh.Vertices.push_back(BeFullVertex{.Position = c, .Normal = n, .Color = color, .UV0 = uvC, .Tangent = tangent});
        mesh.Vertices.push_back(BeFullVertex{.Position = d, .Normal = n, .Color = color, .UV0 = uvD, .Tangent = tangent});
        mesh.Indices.insert(mesh.Indices.end(), {base + 0, base + 1, base + 2, base + 0, base + 2, base + 3});
    }

    auto CreateCardMesh() -> std::shared_ptr<BeMesh> {
        auto mesh = std::make_shared<BeMesh>();
        constexpr float left = -0.5f, right = 0.5f, bottom = -0.5f, top = 0.5f, frontZ = -0.026f, backZ = 0.026f;
        // UVs transcribed EXACTLY from the working fork — top verts get V=1, bottom V=0.
        AddCardQuad(*mesh,
            {left,top,frontZ},{right,top,frontZ},{right,bottom,frontZ},{left,bottom,frontZ}, {0,0,-1},
            {0,1},{1,1},{1,0},{0,0});
        AddCardQuad(*mesh,
            {right,top,backZ},{left,top,backZ},{left,bottom,backZ},{right,bottom,backZ}, {0,0,1},
            {0,1},{1,1},{1,0},{0,0});
        AddCardQuad(*mesh,
            {left,top,frontZ},{right,top,frontZ},{right,top,backZ},{left,top,backZ}, {0,1,0},
            {0,1},{1,1},{1,1},{0,1});
        AddCardQuad(*mesh,
            {right,bottom,frontZ},{left,bottom,frontZ},{left,bottom,backZ},{right,bottom,backZ}, {0,-1,0},
            {1,0},{0,0},{0,0},{1,0});
        AddCardQuad(*mesh,
            {left,bottom,frontZ},{left,top,frontZ},{left,top,backZ},{left,bottom,backZ}, {-1,0,0},
            {0,0},{0,1},{0,1},{0,0});
        AddCardQuad(*mesh,
            {right,top,frontZ},{right,bottom,frontZ},{right,bottom,backZ},{right,top,backZ}, {1,0,0},
            {1,1},{1,0},{1,0},{1,1});
        mesh->Slices.push_back({.IndexCount = static_cast<uint32_t>(mesh->Indices.size()), .StartIndexLocation = 0, .BaseVertexLocation = 0});
        return mesh;
    }

    auto CreateSleeveMesh() -> std::shared_ptr<BeMesh> {
        auto mesh = std::make_shared<BeMesh>();
        constexpr float l = -0.5f, r = 0.5f, b = -0.5f, t = 0.5f;
        AddCardQuad(*mesh, {l,t,-0.04f},{r,t,-0.04f},{r,b,-0.04f},{l,b,-0.04f}, {0,0,-1}, {0,1},{1,1},{1,0},{0,0});
        AddCardQuad(*mesh, {r,t,0.04f},{l,t,0.04f},{l,b,0.04f},{r,b,0.04f}, {0,0,1}, {0,1},{1,1},{1,0},{0,0});
        mesh->Slices.push_back({.IndexCount = static_cast<uint32_t>(mesh->Indices.size()), .StartIndexLocation = 0, .BaseVertexLocation = 0});
        return mesh;
    }

    auto CreateButtonPuckMesh(uint32_t segments = 72) -> std::shared_ptr<BeMesh> {
        auto mesh = std::make_shared<BeMesh>();
        constexpr float pi = 3.14159265359f;
        const glm::vec4 color = {1,1,1,1}, tangent = {1,0,0,1};
        auto addVertex = [&](glm::vec3 p, glm::vec3 nrm, glm::vec2 uv) -> uint32_t {
            const uint32_t i = static_cast<uint32_t>(mesh->Vertices.size());
            mesh->Vertices.push_back(BeFullVertex{.Position = p, .Normal = glm::normalize(nrm), .Color = color, .UV0 = uv, .Tangent = tangent});
            return i;
        };
        struct Ring { float Radius, Y; glm::vec3 NormalBias; };
        const Ring rings[] = {
            {0.30f, 0.50f, {0.00f, 1.00f, 0}}, {0.45f, 0.43f, {0.35f, 0.92f, 0}},
            {0.50f, 0.26f, {0.92f, 0.36f, 0}}, {0.50f, -0.30f, {1.00f, 0.00f, 0}},
            {0.42f, -0.48f, {0.50f, -0.72f, 0}},
        };
        const uint32_t topCenter = addVertex({0, rings[0].Y, 0}, {0,1,0}, {0.5f,0.5f});
        const uint32_t bottomCenter = addVertex({0, rings[4].Y, 0}, {0,-1,0}, {0.5f,0.5f});
        auto ringVertex = [&](const Ring& ring, float angle, float u) -> uint32_t {
            const float x = std::cos(angle) * ring.Radius, z = std::sin(angle) * ring.Radius;
            glm::vec3 nrm = {std::cos(angle) * ring.NormalBias.x, ring.NormalBias.y, std::sin(angle) * ring.NormalBias.x};
            if (glm::dot(nrm, nrm) < 0.0001f) nrm = {0,1,0};
            return addVertex({x, ring.Y, z}, nrm, {u, ring.Y + 0.5f});
        };
        for (uint32_t i = 0; i < segments; ++i) {
            const float a0 = (float(i) / float(segments)) * pi * 2.0f, a1 = (float(i+1) / float(segments)) * pi * 2.0f;
            const float u0 = float(i) / float(segments), u1 = float(i+1) / float(segments);
            const uint32_t top0 = ringVertex(rings[0], a0, u0), top1 = ringVertex(rings[0], a1, u1);
            const uint32_t bottom0 = ringVertex(rings[4], a0, u0), bottom1 = ringVertex(rings[4], a1, u1);
            mesh->Indices.insert(mesh->Indices.end(), {topCenter, top1, top0, bottomCenter, bottom0, bottom1});
            constexpr size_t ringCount = sizeof(rings) / sizeof(rings[0]);
            for (size_t ring = 0; ring < ringCount - 1; ++ring) {
                const uint32_t r0a = ringVertex(rings[ring], a0, u0), r0b = ringVertex(rings[ring], a1, u1);
                const uint32_t r1a = ringVertex(rings[ring+1], a0, u0), r1b = ringVertex(rings[ring+1], a1, u1);
                mesh->Indices.insert(mesh->Indices.end(), {r0a, r1a, r1b, r0a, r1b, r0b});
            }
        }
        mesh->Slices.push_back({.IndexCount = static_cast<uint32_t>(mesh->Indices.size()), .StartIndexLocation = 0, .BaseVertexLocation = 0});
        return mesh;
    }

    auto RotationFacingTarget(const glm::vec3& position, const glm::vec3& target) -> glm::quat {
        glm::vec3 dir = target - position;
        dir.y = 0.0f;
        if (glm::dot(dir, dir) < 0.0001f) return glm::quat();
        dir = glm::normalize(dir);
        return glm::quat(glm::vec3(0.0f, glm::atan(-dir.x, -dir.z), 0.0f));
    }

    auto NeutralCardRotation(const glm::vec3& position) -> glm::quat {
        constexpr float halfPi = 1.57079632679f;
        return glm::quat(glm::vec3(0.0f, position.x < 0.0f ? -halfPi : halfPi, 0.0f));
    }

    auto RayIntersectsAabb(const glm::vec3& origin, const glm::vec3& dir,
        const glm::vec3& center, const glm::vec3& half, float maxDist) -> bool {
        float tMin = 0.0f, tMax = maxDist;
        const glm::vec3 mn = center - half, mx = center + half;
        for (int axis = 0; axis < 3; ++axis) {
            if (glm::abs(dir[axis]) < 0.0001f) {
                if (origin[axis] < mn[axis] || origin[axis] > mx[axis]) return false;
                continue;
            }
            float t1 = (mn[axis] - origin[axis]) / dir[axis], t2 = (mx[axis] - origin[axis]) / dir[axis];
            if (t1 > t2) std::swap(t1, t2);
            tMin = glm::max(tMin, t1);
            tMax = glm::min(tMax, t2);
            if (tMin > tMax) return false;
        }
        return tMax >= 0.0f && tMin <= maxDist;
    }

    auto PlaceCard(size_t index, const BeTexture& texture) -> MemoryPalaceLobby::MemoryCard {
        const float aspect = texture.Height > 0 ? float(texture.Width) / float(texture.Height) : 1.4f;
        const float cardHeight = 2.35f;
        const float cardWidth = glm::clamp(cardHeight * aspect, 1.6f, 4.2f);
        const bool leftSide = (index % 2) == 0;
        const size_t row = index / 2;
        const float x = leftSide ? -3.35f : 3.35f;
        const float yaw = glm::radians(leftSide ? -72.0f : 72.0f);
        const float z = 1.5f + float(row) * 4.4f;
        return MemoryPalaceLobby::MemoryCard{
            .Position = glm::vec3(x, 2.05f, z),
            .Rotation = glm::quat(glm::vec3(0.0f, yaw, 0.0f)),
            .Scale = glm::vec3(cardWidth, cardHeight, 1.0f),
        };
    }

    // Custom pass that draws each card's glowing sleeve over the HDR target with
    // additive blending. Ported from the standalone palace to the new BePass API.
    class MemorySleevePass final : public BeRenderPass {
    public:
        MemorySleevePass(BeStandardRenderMachine* srm,
            std::shared_ptr<BeTexture> colorTarget, std::shared_ptr<BeTexture> depthTarget,
            const std::vector<MemoryPalaceLobby::MemoryCard>* cards)
            : _srm(srm), _colorTarget(std::move(colorTarget)), _depthTarget(std::move(depthTarget)), _cards(cards) {}

        auto Initialise() -> void override {}
        auto GetPassName() const -> const std::string override { return "MemorySleevePass"; }

        auto Render() -> void override {
            if (!_cards || _cards->empty()) return;
            auto& cmd = _renderer->GetCommandBuffer();
            const auto uniformMat = _srm->UniformMaterial.lock();

            cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

            BePass pass;
            pass.AddColorTarget(_colorTarget, SenLoadOp::Load);
            pass.SetDepthTarget(_depthTarget, SenLoadOp::Load);
            pass.SetViewport({0, 0, float(_renderer->GetSwapchainPixelWidth()), float(_renderer->GetSwapchainPixelHeight()), 0, 1});
            pass.Begin();

            cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer());
            cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());

            constexpr SenDepthStencilState depthReadOnly{
                .DepthEnable = true, .DepthWriteEnable = false, .DepthFunc = SenComparisonFunc::LessEqual,
            };

            for (const auto& card : *_cards) {
                auto mat = _srm->AcquireNewObjectMaterial();
                mat->SetMatrix("Model", BeSRMGeometryEntry::CalculateModelMatrix(card.Position, card.Rotation, card.Scale));
                mat->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
                mat->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));
                cmd.SetBindGroup(mat->GetBindGroup(), 1);

                const auto& meshSlices = _srm->GetMeshSlices(card.SleeveProp->Mesh.get());
                for (size_t j = 0; j < meshSlices.size(); ++j) {
                    const auto& meshSlice = meshSlices[j];
                    auto& propSlice = card.SleeveProp->Slices[j];
                    auto pipeline = BePipelineBuilder::Start(*card.SleeveProp->Shader)
                        .SetCullMode(SenCullMode::None)
                        .SetColorFormats({_colorTarget->Format})
                        .SetDepthFormat(_depthTarget->Format)
                        .SetDepthStencil(depthReadOnly)
                        .Build();
                    cmd.SetPipeline(pipeline);
                    cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
                    cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
                }
            }
            pass.End();
        }

    private:
        BeStandardRenderMachine* _srm;
        std::shared_ptr<BeTexture> _colorTarget;
        std::shared_ptr<BeTexture> _depthTarget;
        const std::vector<MemoryPalaceLobby::MemoryCard>* _cards;
    };

    // Button hit volume + placement (shared between Prepare/Tick).
    constexpr glm::vec3 kButtonPosition = {0.0f, 0.055f, -3.05f};
    constexpr glm::vec3 kButtonScale = {1.35f, 0.11f, 1.35f};
    constexpr glm::vec3 kButtonHitCenter = {0.0f, 0.18f, -3.05f};
    constexpr glm::vec3 kButtonHitHalf = {0.95f, 0.42f, 0.95f};
    constexpr glm::vec3 kKnobScale = {0.74f, 0.18f, 0.74f};
}

MemoryPalaceLobby::MemoryPalaceLobby(App* app) : Lobby(app, "memory-palace") {
    _defaultSpawnPos = {0.0f, 1.75f, -6.0f};
    _defaultSpawnLook = {0.0f, 1.95f, 0.0f};
    // Portal back to outbound, placed at the near end of the corridor behind spawn.
    _portals.push_back({.TargetLobby = "outbound", .Position = {0.0f, 0.0f, -9.0f}, .Radius = 1.6f, .Label = "Outbound"});
}

MemoryPalaceLobby::~MemoryPalaceLobby() = default;

auto MemoryPalaceLobby::Prepare() -> void {
    if (_prepared) return;
    _prepared = true;

    const auto root = _app->Assets()->LobbyRoot("memory-palace");
    const auto shaderDir = root / "shaders";
    SenShaderCompiler::AssetShadersPath = shaderDir.string() + "/";

    const auto sp = [&](const char* f) { return shaderDir / f; };
    BeAssetRegistry::IndexShaderFiles({
        sp("uniform-material.hlsl"), sp("objectMaterial.hlsl"),
        sp("memory-card.hlsl"), sp("memory-sleeve.hlsl"), sp("memory-floor.hlsl"), sp("memory-button.hlsl"),
        sp("fullscreen-vertex.hlsl"), sp("directionalLight.hlsl"), sp("pointLight.hlsl"), sp("emissive-add.hlsl"),
        sp("bloom-bright.hlsl"), sp("bloom-downsample.hlsl"), sp("bloom-upsample.hlsl"), sp("bloom-add.hlsl"),
        sp("backbuffer.hlsl"), sp("fxaa.hlsl"),
    });

    _camera = std::make_unique<BeCamera>();
    _camera->Width = _app->Width();
    _camera->Height = _app->Height();
    _camera->NearPlane = 0.05f;
    _camera->FarPlane = 150.0f;
    _camera->Fov = 65.0f;
    _camera->Position = _defaultSpawnPos;
    {
        const glm::vec3 d = glm::normalize(_defaultSpawnLook - _camera->Position);
        _camera->Yaw = glm::degrees(glm::atan(d.z, d.x));
        _camera->Pitch = glm::degrees(glm::asin(d.y));
        _camera->Update();
    }

    _uniformMaterial = BeMaterial::Create("uniform-material", false);
    _uniformMaterial->SetFloat3("AmbientColor", glm::vec3(0.82f));
    _uniformMaterial->SetFloat3("WorldColor", glm::vec3(0.018f, 0.018f, 0.020f));

    _machine = std::make_unique<BeStandardRenderMachine>(_app->Renderer(), _app->Width(), _app->Height());

    _cardShader = BeAssetRegistry::GetShader("memory-card");
    _sleeveShader = BeAssetRegistry::GetShader("memory-sleeve");
    const auto floorShader = BeAssetRegistry::GetShader("memory-floor");
    const auto buttonShader = BeAssetRegistry::GetShader("memory-button");

    _cardMesh = CreateCardMesh();
    _sleeveMesh = CreateSleeveMesh();
    _buttonMesh = CreateButtonPuckMesh();

    _floor = BeProp::FromMesh(BeMeshPrimitives::Plane(32), floorShader, "geometry-main");
    _floor->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.095f, 0.105f, 0.115f));
    _floor->Materials[0]->SetFloat3("WalkwayColor", glm::vec3(0.16f, 0.165f, 0.16f));
    _floor->Materials[0]->SetFloat3("GridColor", glm::vec3(0.23f, 0.235f, 0.225f));
    _floor->Materials[0]->SetFloat("WalkwayWidth", 2.65f);
    _floor->Materials[0]->SetFloat("GridScale", 1.0f);
    _floor->Materials[0]->SetFloat("GridLineWidth", 0.025f);

    _buttonBase = BeProp::FromMesh(_buttonMesh, buttonShader, "geometry-main");
    _buttonBase->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.036f, 0.038f, 0.044f));
    _buttonBase->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.0f));
    _buttonBase->Materials[0]->SetFloat("EmissiveStrength", 0.0f);

    _buttonKnob = BeProp::FromMesh(_buttonMesh, buttonShader, "geometry-main");
    _buttonKnob->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.16f, 0.86f, 0.48f));
    _buttonKnob->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.16f, 0.86f, 0.48f));
    _buttonKnob->Materials[0]->SetFloat("EmissiveStrength", 0.65f);

    // Glowing portal pillar (walk into it to teleport).
    _portalMarker = BeProp::FromMesh(BeMeshPrimitives::Cube(), buttonShader, "geometry-main");
    _portalMarker->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.1f, 0.5f, 1.0f));
    _portalMarker->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.25f, 0.7f, 1.6f));
    _portalMarker->Materials[0]->SetFloat("EmissiveStrength", 1.0f);

    _machine->RegisterMesh(_cardMesh);
    _machine->RegisterMesh(_sleeveMesh);
    _machine->RegisterMesh(_floor->Mesh);
    _machine->RegisterMesh(_buttonMesh);
    _machine->RegisterMesh(_portalMarker->Mesh);

    // Floating destination signs above each portal (the card shader samples
    // CardTexture).
    BuildPortalLabels(_cardShader, "CardTexture");

    // Initial cards from the bundled cards/ dir.
    const auto cardsDir = root / "cards";
    std::vector<std::filesystem::path> images;
    if (std::filesystem::exists(cardsDir)) {
        for (const auto& e : std::filesystem::directory_iterator(cardsDir)) {
            if (e.is_regular_file() && AssetManager::IsSupportedAsset(e.path())) images.push_back(e.path());
        }
        std::ranges::sort(images);
    }
    for (const auto& img : images) AddCardFromImage(img);

    _machine->BakeMeshes();
    _machine->DeclareGBufferTarget("Memory_BaseColor", SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Memory_WorldNormal", SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Memory_ORM", SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Memory_Emissive", SenFormat::R11G11B10_Float);
    _machine->DeclareDepth("Memory_Depth", SenFormat::Depth32);
    _machine->DeclareTexture("Memory_HDR", SenFormat::R11G11B10_Float);
    _machine->DeclareTexture("Memory_Bloom", SenFormat::R11G11B10_Float, 1.0f, false, 5);
    _machine->DeclareTexture("Memory_FXAA", SenFormat::R11G11B10_Float);

    _machine->UniformMaterial = _uniformMaterial;
}

auto MemoryPalaceLobby::AddCardFromImage(const std::filesystem::path& imagePath) -> void {
    try {
        const auto texture = BeTexture::Create(SafeTextureName(imagePath, _cards.size()))
            .LoadFromFile(imagePath).Build();

        auto prop = BeProp::FromMesh(_cardMesh, _cardShader, "geometry-main");
        prop->Slices[0].TwoSided = true;
        prop->Materials[0]->SetTexture("CardTexture", texture);
        prop->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-clamp"));
        prop->Materials[0]->SetFloat("EmissiveStrength", 1.0f);

        auto sleeve = BeProp::FromMesh(_sleeveMesh, _sleeveShader, "geometry-main");
        sleeve->Slices[0].TwoSided = true;
        sleeve->Materials[0]->SetFloat3("GlowColor", glm::vec3(1.0f));
        sleeve->Materials[0]->SetFloat("GlowStrength", 1.45f);
        sleeve->Materials[0]->SetFloat3("SunDirection", glm::normalize(glm::vec3(-0.45f, -1.0f, -0.25f)));

        const auto placement = PlaceCard(_cards.size(), *texture);
        _cards.push_back({
            .Prop = prop, .SleeveProp = sleeve,
            .Position = placement.Position, .Rotation = placement.Rotation, .Scale = placement.Scale,
        });
    } catch (const std::exception& e) {
        std::cerr << "zeen[memory-palace]: skipping card " << imagePath << ": " << e.what() << '\n';
    }
}

auto MemoryPalaceLobby::OnEnter() -> void {
    _machine->ClearPasses();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Memory_HDR");
    _machine->AddPass(std::make_unique<MemorySleevePass>(
        _machine.get(), _machine->GetRenderTexture("Memory_HDR"), _machine->GetRenderTexture("Memory_Depth"), &_cards));
    _machine->AddBloomPass(5, "Memory_HDR", "Memory_Bloom", BeAssetRegistry::GetTexture("black").lock());

    auto fxaaMaterial = BeMaterial::Create("fxaa-material", false);
    fxaaMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Memory_Bloom"));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("fxaa"), fxaaMaterial, {"Memory_FXAA"});
    _machine->AddBackbufferPass("Memory_FXAA", {0.018f, 0.018f, 0.020f});
    _machine->BuildPasses();

    // Drag-drop a new card while in this lobby.
    auto* self = this;
    _app->Assets()->SetDropHandler([self](const std::filesystem::path& p) {
        self->AddCardFromImage(p);
        self->_machine->BakeMeshes();   // card mesh is shared+already baked; rebake is cheap and safe
    });
}

auto MemoryPalaceLobby::Tick(float deltaTime) -> void {
    UpdateFreeCamera(deltaTime);

    if (_app->Input()->GetKeyDown(GLFW_KEY_BACKSPACE)) {
        if (_app->Lobbies()->GoBack()) return;
    }
    if (CheckPortals()) return;

    SubmitFrame(static_cast<float>(glfwGetTime()));
}

auto MemoryPalaceLobby::SubmitFrame(float now) -> void {
    auto& input = *_app->Input();

    const bool buttonHovered = RayIntersectsAabb(
        _camera->Position, glm::normalize(_camera->GetFront()), kButtonHitCenter, kButtonHitHalf, 8.0f);
    if (buttonHovered && input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
        _cardsFollowCamera = !_cardsFollowCamera;
    }

    for (auto& card : _cards) {
        card.Rotation = _cardsFollowCamera
            ? RotationFacingTarget(card.Position, _camera->Position)
            : NeutralCardRotation(card.Position);
    }

    const glm::vec3 knobColor = _cardsFollowCamera ? glm::vec3(0.16f, 0.86f, 0.48f) : glm::vec3(0.34f, 0.36f, 0.40f);
    const glm::vec3 baseColor = buttonHovered ? glm::vec3(0.070f, 0.074f, 0.084f) : glm::vec3(0.036f, 0.038f, 0.044f);
    const glm::vec3 knobPosition = {kButtonPosition.x, _cardsFollowCamera ? 0.19f : 0.13f, kButtonPosition.z};

    _buttonBase->Materials[0]->SetFloat3("BaseColor", baseColor);
    _buttonBase->Materials[0]->SetFloat3("EmissiveColor", buttonHovered ? glm::vec3(0.20f, 0.24f, 0.23f) : glm::vec3(0.0f));
    _buttonBase->Materials[0]->SetFloat("EmissiveStrength", buttonHovered ? 0.16f : 0.0f);
    _buttonKnob->Materials[0]->SetFloat3("BaseColor", knobColor);
    _buttonKnob->Materials[0]->SetFloat3("EmissiveColor", knobColor);
    _buttonKnob->Materials[0]->SetFloat("EmissiveStrength", _cardsFollowCamera ? 0.62f : 0.10f);

    const auto pv = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    _uniformMaterial->SetMatrix("CameraProjectionView", pv);
    _uniformMaterial->SetMatrix("CameraInverseProjectionView", glm::inverse(pv));
    _uniformMaterial->SetFloat4("NearFarPlane", {_camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane});
    _uniformMaterial->SetFloat3("CameraPosition", _camera->Position);
    _uniformMaterial->SetFloat("Time", now);

    _machine->ClearFrame();
    _machine->AddGeometry({
        .Name = "floor",
        .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix({0.0f, 0.0f, 9.0f}, glm::quat(), {18.0f, 1.0f, 42.0f}),
        .Prop = _floor,
    });
    _machine->AddGeometry({.Name = "btn-base", .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(kButtonPosition, glm::quat(), kButtonScale), .Prop = _buttonBase});
    _machine->AddGeometry({.Name = "btn-knob", .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(knobPosition, glm::quat(), kKnobScale), .Prop = _buttonKnob});

    // Glowing portal pillars — walk into one to teleport.
    int pi = 0;
    for (const auto& portal : _portals) {
        const float bob = 0.12f * glm::sin(now * 2.0f + static_cast<float>(pi));
        _machine->AddGeometry({
            .Name = "portal-" + portal.TargetLobby,
            .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(
                {portal.Position.x, 1.6f + bob, portal.Position.z},
                glm::quat(glm::vec3(0.0f, now * 0.6f, 0.0f)), {0.4f, 3.2f, 0.4f}),
            .Prop = _portalMarker,
        });
        ++pi;
    }
    SubmitPortalLabels(now);

    for (size_t i = 0; i < _cards.size(); ++i) {
        _machine->AddGeometry({
            .Name = "card-" + std::to_string(i),
            .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(_cards[i].Position, _cards[i].Rotation, _cards[i].Scale),
            .Prop = _cards[i].Prop,
        });
    }

    _machine->AddSunLight({
        .Direction = glm::normalize(glm::vec3(-0.45f, -1.0f, -0.25f)),
        .Color = glm::vec3(1.0f), .Power = 7.5f, .CastsShadows = false,
        .ShadowViewProjection = glm::mat4(1.0f), .ShadowMapResolution = 1,
        .ShadowMap = BeAssetRegistry::GetTexture("black"),
    });
    _machine->AddPointLight({
        .Name = "front-key", .Position = glm::vec3(0.0f, 3.2f, -2.75f), .Radius = 11.0f,
        .Color = glm::vec3(0.90f, 0.97f, 1.0f), .Power = 7.0f, .CastsShadows = false,
        .ShadowMapResolution = 1, .ShadowNearPlane = 0.1f, .ShadowMap = BeAssetRegistry::GetTexture("black-cube"),
    });
    _machine->AddPointLight({
        .Name = "corridor-fill", .Position = glm::vec3(0.0f, 2.6f, 6.0f), .Radius = 16.0f,
        .Color = glm::vec3(1.0f, 0.96f, 0.90f), .Power = 3.0f, .CastsShadows = false,
        .ShadowMapResolution = 1, .ShadowNearPlane = 0.1f, .ShadowMap = BeAssetRegistry::GetTexture("black-cube"),
    });
}
