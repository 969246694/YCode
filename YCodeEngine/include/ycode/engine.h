#ifndef YCODE_ENGINE_H
#define YCODE_ENGINE_H

#include "ycode/event_bus.h"
#include "ycode/plugin_loader.h"
#include "ycode/version.h"

#include <chrono>
#include <string>

namespace ycode {

struct EngineConfig {
    std::string appName = "YCode Game";
    std::string pluginDirectory = "plugins";
    int targetFps = 60;
};

class Engine {
public:
    explicit Engine(EngineConfig config = {});
    ~Engine();

    bool initialize(std::string* error = nullptr);
    void tick();
    void shutdown();

    bool loadPlugin(const std::string& path, std::string* error = nullptr);

    bool isRunning() const;
    const EngineConfig& config() const;
    EventBus& events();
    const EventBus& events() const;
    PluginLoader& plugins();
    const PluginLoader& plugins() const;

private:
    EngineConfig config_;
    EventBus eventBus_;
    PluginLoader pluginLoader_;
    bool running_ = false;
    std::chrono::steady_clock::time_point lastTick_;
};

} // namespace ycode

#endif // YCODE_ENGINE_H

