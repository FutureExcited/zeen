#include "PbrRoomLobby.h"

#include <iostream>

#include <umbrellas/include-glfw.h>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "sen-rhi/SenShaderCompiler.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

#include "App.h"
#include "AssetManager.h"
#include "LobbyManager.h"

PbrRoomLobby::PbrRoomLobby(App* app, std::string name) : Lobby(app, std::move(name)) {}

auto PbrRoomLobby::Shader() const -> std::weak_ptr<BeShader> { return _shader; }

auto PbrRoomLobby::Prepare() -> void {
    if (_prepared) return;
    _prepared = true;

    const auto root = _app->Assets()->LobbyRoot(_name);
    const auto shaderDir = root / "shaders";
    SenShaderCompiler::AssetShadersPath = shaderDir.string() + "/";

    const auto sp = [&](const char* f) { return shaderDir / f; };
    BeAssetRegistry::IndexShaderFiles({
        sp("uniform-material.hlsl"), sp("objectMaterial.hlsl"), sp("zoro-pbr.hlsl"),
        sp("fullscreen-vertex.hlsl"), sp("directionalLight.hlsl"), sp("pointLight.hlsl"),
        sp("emissive-add.hlsl"), sp("backbuffer.hlsl"), sp("fxaa.hlsl"),
    });

    _camera = std::make_unique<BeCamera>();
    _camera->Width = _app->Width();
    _camera->Height = _app->Height();
    _camera->NearPlane = 0.05f;
    _camera->FarPlane = 100.0f;
    _camera->Fov = 65.0f;
    _camera->Position = _defaultSpawnPos;
    {
        const glm::vec3 lookDir = glm::normalize(_defaultSpawnLook - _camera->Position);
        _camera->Yaw = glm::degrees(glm::atan(lookDir.z, lookDir.x));
        _camera->Pitch = glm::degrees(glm::asin(lookDir.y));
        _camera->Update();
    }

    _uniformMaterial = BeMaterial::Create("uniform-material", false);
    _uniformMaterial->SetFloat3("AmbientColor", AmbientColor());
    _uniformMaterial->SetFloat3("WorldColor", glm::vec3(1.0f));

    _machine = std::make_unique<BeStandardRenderMachine>(_app->Renderer(), _app->Width(), _app->Height());
    _shader = BeAssetRegistry::GetShader("zoro-pbr");

    _floor = BeProp::FromMesh(BeMeshPrimitives::Plane(24), _shader, "geometry-main");
    _floor->Materials[0]->SetFloat3("BaseColor", FloorColor());
    _floor->Materials[0]->SetFloat("Roughness", 0.9f);
    _machine->RegisterMesh(_floor->Mesh);

    // Subclass loads its props (registers meshes) before the single bake.
    BuildContent();

    _machine->BakeMeshes();
    _machine->DeclareGBufferTarget(Tex("BaseColor"), SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget(Tex("WorldNormal"), SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget(Tex("ORM"), SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget(Tex("Emissive"), SenFormat::R11G11B10_Float);
    _machine->DeclareDepth(Tex("Depth"), SenFormat::Depth32);
    _machine->DeclareTexture(Tex("HDR"), SenFormat::R11G11B10_Float);
    _machine->DeclareTexture(Tex("FXAA"), SenFormat::R11G11B10_Float);

    _machine->UniformMaterial = _uniformMaterial;
    _uploads = std::make_unique<UploadSpawner>(_app, _machine.get(), _shader);
}

auto PbrRoomLobby::OnEnter() -> void {
    _machine->ClearPasses();
    _machine->AddGeometryPass();
    _machine->AddLightingPass(Tex("HDR"));

    auto fxaaMaterial = BeMaterial::Create("fxaa-material", false);
    fxaaMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture(Tex("HDR")));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("fxaa"), fxaaMaterial, { Tex("FXAA") });
    _machine->AddBackbufferPass(Tex("FXAA"), { 1.0f, 1.0f, 1.0f });
    _machine->BuildPasses();

    auto* uploads = _uploads.get();
    auto* cameraPtr = _camera.get();
    _app->Assets()->SetDropHandler([uploads, cameraPtr](const std::filesystem::path& p) {
        uploads->Spawn(p, *cameraPtr);
    });
}

auto PbrRoomLobby::Tick(float deltaTime) -> void {
    UpdateFreeCamera(deltaTime);

    if (_app->Input()->GetKeyDown(GLFW_KEY_BACKSPACE)) {
        if (_app->Lobbies()->GoBack()) return;
    }
    if (CheckPortals()) return;

    const float now = static_cast<float>(glfwGetTime());
    const auto pv = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    _uniformMaterial->SetMatrix("CameraProjectionView", pv);
    _uniformMaterial->SetMatrix("CameraInverseProjectionView", glm::inverse(pv));
    _uniformMaterial->SetFloat4("NearFarPlane",
        { _camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane });
    _uniformMaterial->SetFloat3("CameraPosition", _camera->Position);
    _uniformMaterial->SetFloat("Time", now);

    _machine->ClearFrame();
    _machine->AddGeometry({
        .Name = "floor",
        .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix({0.0f, 0.0f, 0.0f}, glm::quat(), {12.0f, 1.0f, 12.0f}),
        .Prop = _floor,
    });
    SubmitContent(now);
    _uploads->Submit();
    SubmitLights();
}

auto PbrRoomLobby::SubmitLights() -> void {
    _machine->AddSunLight({
        .Direction = glm::normalize(glm::vec3(-0.45f, -1.0f, -0.25f)),
        .Color = glm::vec3(1.0f), .Power = 9.5f, .CastsShadows = false,
        .ShadowViewProjection = glm::mat4(1.0f), .ShadowMapResolution = 1,
        .ShadowMap = BeAssetRegistry::GetTexture("black"),
    });
    _machine->AddPointLight({
        .Name = "fill", .Position = glm::vec3(0.0f, 2.2f, -2.65f), .Radius = 6.0f,
        .Color = glm::vec3(0.96f, 0.98f, 1.0f), .Power = 5.0f, .CastsShadows = false,
        .ShadowMapResolution = 1, .ShadowNearPlane = 0.1f,
        .ShadowMap = BeAssetRegistry::GetTexture("black-cube"),
    });
}
