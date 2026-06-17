#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <umbrellas/include-glm.h>

class App;
class BeProp;
class BeShader;
class BeStandardRenderMachine;
class BeCamera;

// Turns user-uploaded files (dropped onto the window while in a lobby) into
// objects placed in front of the player:
//   - images (png/jpg/...) -> a textured quad standing upright
//   - models (gltf/glb/obj) -> loaded as a prop
//
// Each lobby owns an UploadSpawner bound to its render machine. Newly uploaded
// meshes are registered and the machine re-bakes its shared buffers so the object
// shows up the same frame. Spawned objects are submitted every frame via Submit().
class UploadSpawner {
public:
    UploadSpawner(App* app, BeStandardRenderMachine* machine, std::weak_ptr<BeShader> shader);

    // Called by the AssetManager drop handler with a stored upload path.
    auto Spawn(const std::filesystem::path& assetPath, const BeCamera& camera) -> void;

    // Submit all spawned objects into the current frame.
    auto Submit() -> void;

    auto Count() const -> size_t { return _objects.size(); }

private:
    struct Spawned {
        std::shared_ptr<BeProp> prop;
        glm::vec3 position{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        std::string name;
    };

    auto SpawnImage(const std::filesystem::path& path, const BeCamera& camera) -> void;
    auto SpawnModel(const std::filesystem::path& path, const BeCamera& camera) -> void;
    auto PlaceInFrontOf(const BeCamera& camera, float distance) const -> glm::vec3;

    App* _app = nullptr;
    BeStandardRenderMachine* _machine = nullptr;
    std::weak_ptr<BeShader> _shader;
    std::vector<Spawned> _objects;
    int _counter = 0;
};
