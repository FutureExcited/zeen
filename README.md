# zeen

A walkable multi-lobby 3D space built on [be-engine](https://github.com/progdruid/be-engine).
Walk into portals to move between lobbies; it remembers where you came from and
reopens in your last-visited lobby. Drag files onto the window to drop them into
the world.

## Layout

```
zeen/
├── external/be-engine/   # be-engine as a git submodule (pinned to a zeen-macos branch)
├── src/                  # the zeen app
│   ├── App.{h,cpp}            # host: window/renderer/input + main loop
│   ├── Lobby.{h,cpp}          # base lobby: camera, portals, spawn logic
│   ├── LobbyManager.{h,cpp}   # walk in/out, history stack, last-lobby persistence
│   ├── AssetManager.{h,cpp}   # asset roots + drag-drop upload ingestion
│   ├── UploadSpawner.{h,cpp}  # turns an uploaded file into an in-world object
│   ├── lobbies/               # PbrRoomLobby base + Zoro / MemoryPalace / Outbound
│   └── assets/lobbies/<name>/ # per-lobby shaders + models
└── patches/macos-support.patch  # the macOS port re-applied onto upstream be-engine
```

## Lobbies

- **zoro** — character model in a room (the hub).
- **memory-palace** — a floating orb room.
- **outbound** — a clean staging room with a pedestal.

Each is a `BeScene` subclass registered with `BeSceneManager`; switching is the
engine's crash-safe deferred scene change.

## Controls

- `WASD` move, `E`/`Q` up/down, hold **right mouse** to look, arrows to look with the keyboard.
- Walk into a glowing portal spot to enter another lobby.
- **Backspace** — walk back to the previous lobby.
- **Drag & drop** an image (png/jpg/…) or model (gltf/glb/obj) onto the window to
  spawn it in front of you. Uploads are stored under `~/.local/share/zeen/uploads`.
- `Esc` — quit.

## Building (macOS, Apple Silicon)

be-engine upstream is Linux/Windows-only; zeen carries a small macOS patch on its
submodule branch. One-time dependency setup:

```sh
brew install vulkan-loader molten-vk vulkan-headers vulkan-tools
# slang macOS binaries live in external/be-engine/vendor/slang/macos-aarch64
# (fetch via external/be-engine/tools/setup-macos-deps.sh if missing)
```

Then:

```sh
git submodule update --init --recursive
cmake --preset macos-debug
cmake --build out/macos-debug --target zeen -j8
./out/macos-debug/src/zeen.app/Contents/MacOS/zeen
```

## Updating be-engine

The engine is a submodule on a `zeen-macos` branch (`origin/main` + the macOS
patch). To pull upstream changes:

```sh
cd external/be-engine
git fetch origin
git rebase origin/main zeen-macos   # replay the macOS patch onto latest
# resolve any conflicts (most likely in SenVulkanBackend_Core.cpp), then:
cmake --build ../../out/macos-debug --target zeen -j8
```
