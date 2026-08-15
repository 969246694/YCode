# YCode Engine

YCode Engine is the built-in game development runtime for YCode.

It replaces the old standalone `YiyangzaiEngine` direction with a YCode-owned subsystem:

- Small C++17 core
- Event bus
- Scene, entity, and transform model
- Resource manager and JSON scene loader
- Box2D-backed 2D rigid body physics
- Minimal native window layer
- `Key` input helpers and `Canvas2D` primitive drawing
- Stable C ABI plugin contract
- Cross-platform dynamic plugin loader
- CMake build
- Launcher for smoke tests and examples

## Build

```bat
cd YCodeEngine
build.bat
```

Output:

```text
build\msvc2022_64\Release\ycode_engine_launcher.exe
```

## Tests

Engine 核心（EventBus、Scene、SceneLoader、ResourceManager、PhysicsWorld2D）带有 CTest 单元测试：

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

测试源码位于 `tests/engine_tests.cpp`，会随 CI（`.github/workflows/ci.yml`）在 Ubuntu 与 Windows 上自动运行。

## Architecture

- `include/ycode/engine.h`: engine lifecycle
- `include/ycode/resource_manager.h`: project-root resource lookup and text loading
- `include/ycode/physics2d.h`: 2D rigid body physics facade backed by Box2D
- `include/ycode/input.h`: engine-level key constants
- `include/ycode/canvas2d.h`: tiny 2D drawing wrapper for paint callbacks
- `include/ycode/scene.h`: scene, entity, and 2D transform model
- `include/ycode/scene_loader.h`: JSON scene manifest loader
- `include/ycode/event_bus.h`: publish/subscribe event bus
- `include/ycode/window.h`: native window abstraction
- `include/ycode/plugin.h`: plugin ABI
- `include/ycode/plugin_loader.h`: dynamic plugin loader
- `src/main.cpp`: launcher smoke test

## Third-party

- `third_party/nlohmann/`: vendored `nlohmann/json` headers, MIT license, used by `SceneLoader`.
- `third_party/box2d/`: vendored Box2D 3.1.1 source, MIT license, used by `PhysicsWorld2D`.

## Scene physics

`SceneLoader` supports declarative 2D physics on each entity:

```json
{
  "name": "Player",
  "transform": {
    "position": [0.0, 80.0]
  },
  "physics2D": {
    "bodyType": "dynamic",
    "box": {
      "halfSizeMeters": [0.375, 0.375],
      "fixedRotation": true
    }
  }
}
```

`Engine::loadScene()` automatically clears and rebuilds `PhysicsWorld2D` bodies from these declarations.

Circle colliders are supported via the `circle` field (mutually exclusive with `box`; if both present, `circle` wins):

```json
{
  "name": "Coin",
  "transform": { "position": [5.0, 10.0] },
  "physics2D": {
    "bodyType": "kinematic",
    "circle": { "radiusMeters": 0.3 }
  }
}
```

Capsule colliders (common for characters / bullets) use the `capsule` field (priority: capsule > circle > box):

```json
{
  "name": "Hero",
  "transform": { "position": [8.0, 15.0] },
  "physics2D": {
    "bodyType": "dynamic",
    "capsule": {
      "center1": [0.0, -0.5],
      "center2": [0.0, 0.5],
      "radiusMeters": 0.2
    }
  }
}
```
