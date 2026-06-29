#ifndef YCODE_ENGINE_H
#define YCODE_ENGINE_H

#include "ycode/event_bus.h"
#include "ycode/plugin_loader.h"
#include "ycode/resource_manager.h"
#include "ycode/scene.h"
#include "ycode/version.h"
#include "ycode/window.h"

#include <chrono>
#include <functional>
#include <string>

namespace ycode {

struct EngineConfig {
    std::string appName = "YCode Game";
    std::string projectRoot = ".";
    std::string pluginDirectory = "plugins";
    std::string startupScenePath;
    int targetFps = 60;
    bool createWindow = true;
    bool loadStartupScene = false;
    WindowConfig window;
};

class Engine {
public:
    /// Called every frame after scene.update(), before input state reset.
    /// Use this to issue your own GDI / graphics drawing.
    using RenderHandler = std::function<void()>;

    explicit Engine(EngineConfig config = {});
    ~Engine();

    bool initialize(std::string* error = nullptr);
    void tick();
    void shutdown();

    bool loadPlugin(const std::string& path, std::string* error = nullptr);
    bool loadScene(const std::string& path, std::string* error = nullptr);

    bool isRunning() const;
    const EngineConfig& config() const;
    EventBus& events();
    const EventBus& events() const;
    PluginLoader& plugins();
    const PluginLoader& plugins() const;
    ResourceManager& resources();
    const ResourceManager& resources() const;
    Scene& scene();
    const Scene& scene() const;
    Window& window();
    const Window& window() const;

    void setRenderHandler(RenderHandler handler);

private:
    EngineConfig config_;
    EventBus eventBus_;
    PluginLoader pluginLoader_;
    ResourceManager resources_;
    Scene scene_;
    Window window_;
    bool running_ = false;
    std::chrono::steady_clock::time_point lastTick_;
    RenderHandler renderHandler_;
};

} // namespace ycode

#endif // YCODE_ENGINE_H
