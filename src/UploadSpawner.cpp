#include "UploadSpawner.h"

#include <iostream>

#include <umbrellas/include-glm.h>
#include <glm/gtc/matrix_transform.hpp>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeMaterial.h"
#include "BeMesh.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

#include "App.h"
#include "AssetManager.h"

UploadSpawner::UploadSpawner(App* app, BeStandardRenderMachine* machine, std::weak_ptr<BeShader> shader)
    : _app(app), _machine(machine), _shader(std::move(shader)) {}

auto UploadSpawner::PlaceInFrontOf(const BeCamera& camera, float distance) const -> glm::vec3 {
    glm::vec3 forward = camera.GetFront();
    forward.y = 0.0f;
    if (glm::length(forward) < 0.001f) forward = glm::vec3(0.0f, 0.0f, 1.0f);
    forward = glm::normalize(forward);
    glm::vec3 pos = camera.Position + forward * distance;
    pos.y = 0.0f;   // drop to the floor; image quads stand up from here
    return pos;
}

auto UploadSpawner::Spawn(const std::filesystem::path& assetPath, const BeCamera& camera) -> void {
    std::string ext = assetPath.extension().string();
    for (auto& c : ext) c = static_cast<char>(std::tolower(c));

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
        SpawnImage(assetPath, camera);
    } else if (ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".fbx") {
        SpawnModel(assetPath, camera);
    } else {
        std::cerr << "zeen: don't know how to spawn " << assetPath << '\n';
    }
}

auto UploadSpawner::SpawnImage(const std::filesystem::path& path, const BeCamera& camera) -> void {
    const std::string texName = "upload-tex-" + std::to_string(_counter);
    const std::string propName = "upload-" + std::to_string(_counter);
    ++_counter;

    auto texture = BeTexture::Create(texName)
        .LoadFromFile(path)
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetMipsAuto()
        .GenerateMips()
        .AddToRegistry()
        .Build();
    if (!texture) {
        std::cerr << "zeen: failed to load image " << path << '\n';
        return;
    }

    // A unit quad in the XZ plane; rotate upright so it faces the player.
    auto mesh = BeMeshPrimitives::Plane(1);
    auto prop = BeProp::FromMesh(mesh, _shader, "geometry-main");
    if (prop->Materials.empty()) {
        std::cerr << "zeen: upload prop has no material slot\n";
        return;
    }
    auto& mat = prop->Materials[0];
    mat->SetTexture("Diffuse_or_Albedo", texture);
    mat->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-clamp"));
    mat->SetFloat3("BaseColor", glm::vec3(1.0f));
    mat->SetFloat("Roughness", 0.85f);
    mat->SetFloat("Metallic", 0.0f);

    // Register + re-bake so the new mesh is in the shared buffer this frame.
    _machine->RegisterMesh(mesh);
    _machine->BakeMeshes();

    // Aspect-correct: keep height ~1.6m, width from image aspect.
    const float aspect = texture->Height > 0
        ? static_cast<float>(texture->Width) / static_cast<float>(texture->Height)
        : 1.0f;
    const float height = 1.6f;

    Spawned s;
    s.prop = prop;
    s.position = PlaceInFrontOf(camera, 3.0f);
    s.position.y = height * 0.5f;        // lift so the quad's centre is at half-height
    // Plane normal is +Y; rotate -90° about X to stand upright facing -Z, then yaw to face camera.
    const float yaw = glm::radians(camera.Yaw + 180.0f);
    s.rotation = glm::quat(glm::vec3(glm::radians(-90.0f), yaw, 0.0f));
    s.scale = glm::vec3(height * aspect, 1.0f, height);
    s.name = propName;
    _objects.push_back(std::move(s));

    std::cout << "zeen: spawned image " << path.filename()
              << " (" << texture->Width << "x" << texture->Height << ")\n";
}

auto UploadSpawner::SpawnModel(const std::filesystem::path& path, const BeCamera& camera) -> void {
    auto prop = _machine->LoadProp(path, _shader);
    if (!prop) {
        std::cerr << "zeen: failed to load model " << path << '\n';
        return;
    }
    for (auto& m : prop->Materials) {
        m->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-clamp"));
    }
    _machine->BakeMeshes();

    Spawned s;
    s.prop = prop;
    s.position = PlaceInFrontOf(camera, 3.0f);
    s.rotation = glm::quat(glm::vec3(0.0f, glm::radians(camera.Yaw + 180.0f), 0.0f));
    s.scale = glm::vec3(1.0f);
    s.name = "upload-model-" + std::to_string(_counter++);
    _objects.push_back(std::move(s));

    std::cout << "zeen: spawned model " << path.filename() << '\n';
}

auto UploadSpawner::Submit() -> void {
    for (const auto& obj : _objects) {
        _machine->AddGeometry({
            .Name = obj.name,
            .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(obj.position, obj.rotation, obj.scale),
            .Prop = obj.prop,
        });
    }
}
