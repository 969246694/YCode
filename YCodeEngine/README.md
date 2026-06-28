# YCode Engine

YCode Engine is the built-in game development runtime for YCode.

It replaces the old standalone `YiyangzaiEngine` direction with a YCode-owned subsystem:

- Small C++17 core
- Event bus
- Minimal native window layer
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

## Architecture

- `include/ycode/engine.h`: engine lifecycle
- `include/ycode/event_bus.h`: publish/subscribe event bus
- `include/ycode/window.h`: native window abstraction
- `include/ycode/plugin.h`: plugin ABI
- `include/ycode/plugin_loader.h`: dynamic plugin loader
- `src/main.cpp`: launcher smoke test
