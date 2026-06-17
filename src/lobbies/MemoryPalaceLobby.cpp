#include "MemoryPalaceLobby.h"

#include <umbrellas/include-glm.h>

#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

MemoryPalaceLobby::MemoryPalaceLobby(App* app) : PbrRoomLobby(app, "memory-palace") {
    _defaultSpawnPos = {0.0f, 1.8f, -6.0f};
    _defaultSpawnLook = {0.0f, 1.2f, 0.0f};
    _portals.push_back({ .TargetLobby = "zoro",     .Position = {0.0f, 0.0f, -8.0f}, .Radius = 1.4f, .Label = "Back" });
    _portals.push_back({ .TargetLobby = "outbound", .Position = {6.0f, 0.0f, 0.0f},  .Radius = 1.3f, .Label = "Outbound" });
}

auto MemoryPalaceLobby::BuildContent() -> void {
    _plinth = BeProp::FromMesh(BeMeshPrimitives::Sphere(24, 48), Shader(), "geometry-main");
    _plinth->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.55f, 0.5f, 0.7f));
    _plinth->Materials[0]->SetFloat("Roughness", 0.25f);
    _plinth->Materials[0]->SetFloat("Metallic", 0.6f);
    _machine->RegisterMesh(_plinth->Mesh);
}

auto MemoryPalaceLobby::SubmitContent(float now) -> void {
    // Slowly bob the orb.
    const float y = 1.4f + 0.15f * glm::sin(now * 1.5f);
    _machine->AddGeometry({
        .Name = "orb",
        .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(
            {0.0f, y, 0.0f}, glm::quat(glm::vec3(0.0f, now * 0.3f, 0.0f)), glm::vec3(1.2f)),
        .Prop = _plinth,
    });
}
