#include "ycode/engine.h"
#include "ycode/scene_loader.h"

#include <string>
#include <utility>

namespace ycode {

Engine::Engine(EngineConfig config)
    : config_(std::move(config)),
      resources_(config_.projectRoot)
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

    resources_.setRootPath(config_.projectRoot);

    if (config_.loadStartupScene)
    {
        if (config_.startupScenePath.empty())
        {
            if (error)
                *error = "startupScenePath is empty";
            return false;
        }

        if (!loadScene(config_.startupScenePath, error))
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
    physics_.step(scene_, deltaSeconds);

    // Call user's render handler (if set) after the scene update.
    if (renderHandler_)
        renderHandler_();

    eventBus_.publish({"engine.tick",
                       {{"deltaMs", std::to_string(deltaMs)},
                        {"deltaSeconds", std::to_string(deltaSeconds)},
                        {"entityCount", std::to_string(scene_.entityCount())}}});

    // Reset per-frame input state at the very end of the tick.
    if (config_.createWindow)
        window_.endFrame();
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

bool Engine::loadScene(const std::string& path, std::string* error)
{
    std::string resolvedPath = resources_.resolvePath(path);
    if (!SceneLoader::loadFromFile(resolvedPath, scene_, error))
        return false;

    eventBus_.publish({"engine.scene_loaded",
                       {{"path", path},
                        {"resolvedPath", resolvedPath},
                        {"sceneName", scene_.name()},
                        {"entityCount", std::to_string(scene_.entityCount())}}});
    return true;
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

ResourceManager& Engine::resources()
{
    return resources_;
}

const ResourceManager& Engine::resources() const
{
    return resources_;
}

PhysicsWorld2D& Engine::physics()
{
    return physics_;
}

const PhysicsWorld2D& Engine::physics() const
{
    return physics_;
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

void Engine::setRenderHandler(RenderHandler handler)
{
    renderHandler_ = std::move(handler);
}

} // namespace ycode
