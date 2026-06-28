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

    if (config_.createWindow)
    {
        if (config_.window.title.empty())
            config_.window.title = config_.appName;

        if (!window_.create(config_.window, error))
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

    if (config_.createWindow)
    {
        window_.pollEvents();
        if (!window_.isOpen())
        {
            running_ = false;
            eventBus_.publish({"engine.window_closed", {{"appName", config_.appName}}});
            return;
        }
    }

    auto now = std::chrono::steady_clock::now();
    auto deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count();
    auto deltaSeconds = std::chrono::duration<float>(now - lastTick_).count();
    lastTick_ = now;

    scene_.update(deltaSeconds);
    eventBus_.publish({"engine.tick",
                       {{"deltaMs", std::to_string(deltaMs)},
                        {"deltaSeconds", std::to_string(deltaSeconds)},
                        {"entityCount", std::to_string(scene_.entityCount())}}});
}

void Engine::shutdown()
{
    if (!running_)
        return;

    eventBus_.publish({"engine.shutdown", {{"appName", config_.appName}}});
    pluginLoader_.unloadAll();
    window_.close();
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

Scene& Engine::scene()
{
    return scene_;
}

const Scene& Engine::scene() const
{
    return scene_;
}

Window& Engine::window()
{
    return window_;
}

const Window& Engine::window() const
{
    return window_;
}

} // namespace ycode
