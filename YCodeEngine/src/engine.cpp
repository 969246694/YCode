#include "ycode/engine.h"

#include <string>
#include <utility>

namespace ycode {

Engine::Engine(EngineConfig config)
    : config_(std::move(config))
{
}

Engine::~Engine()
{
    shutdown();
}

bool Engine::initialize(std::string* error)
{
    if (running_)
        return true;

    if (config_.targetFps <= 0)
    {
        if (error)
            *error = "targetFps must be greater than zero";
        return false;
    }

    running_ = true;
    lastTick_ = std::chrono::steady_clock::now();
    eventBus_.publish({"engine.initialized", {{"appName", config_.appName}}});
    return true;
}

void Engine::tick()
{
    if (!running_)
        return;

    auto now = std::chrono::steady_clock::now();
    auto deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count();
    lastTick_ = now;
    eventBus_.publish({"engine.tick", {{"deltaMs", std::to_string(deltaMs)}}});
}

void Engine::shutdown()
{
    if (!running_)
        return;

    eventBus_.publish({"engine.shutdown", {{"appName", config_.appName}}});
    pluginLoader_.unloadAll();
    eventBus_.clear();
    running_ = false;
}

bool Engine::loadPlugin(const std::string& path, std::string* error)
{
    return pluginLoader_.load(path, error);
}

bool Engine::isRunning() const
{
    return running_;
}

const EngineConfig& Engine::config() const
{
    return config_;
}

EventBus& Engine::events()
{
    return eventBus_;
}

const EventBus& Engine::events() const
{
    return eventBus_;
}

PluginLoader& Engine::plugins()
{
    return pluginLoader_;
}

const PluginLoader& Engine::plugins() const
{
    return pluginLoader_;
}

} // namespace ycode
