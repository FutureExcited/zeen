#include "OutboundLobby.h"

#include <umbrellas/include-glm.h>

#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

OutboundLobby::OutboundLobby(App* app) : PbrRoomLobby(app, "outbound") {
    _defaultSpawnPos = {0.0f, 1.8f, -6.0f};
    _defaultSpawnLook = {0.0f, 1.0f, 0.0f};
    // A single portal back to the zoro hub.
    _portals.push_back({ .TargetLobby = "zoro", .Position = {0.0f, 0.0f, -8.0f}, .Radius = 1.4f, .Label = "Back" });
}

auto OutboundLobby::BuildContent() -> void {
    // A low cube pedestal at the room centre.
    _pedestal = BeProp::FromMesh(BeMeshPrimitives::Cube(), Shader(), "geometry-main");
    _pedestal->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.30f, 0.32f, 0.38f));
    _pedestal->Materials[0]->SetFloat("Roughness", 0.4f);
    _pedestal->Materials[0]->SetFloat("Metallic", 0.1f);
    _machine->RegisterMesh(_pedestal->Mesh);
}

auto OutboundLobby::SubmitContent(float now) -> void {
    _machine->AddGeometry({
        .Name = "pedestal",
        .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(
            {0.0f, 0.5f, 0.0f}, glm::quat(), {2.0f, 1.0f, 2.0f}),
        .Prop = _pedestal,
    });
}
