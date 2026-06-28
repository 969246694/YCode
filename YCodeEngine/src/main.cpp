#include "ycode/core.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main()
{
    auto version = ycode::getVersion();
    std::cout << "YCode Engine v" << version.major << "." << version.minor << "." << version.patch << std::endl;
    std::cout << "================================" << std::endl;

    ycode::EngineConfig config;
    config.appName = "YCode Engine Sandbox";
    config.window.title = "YCode Engine Sandbox";
    config.window.width = 1280;
    config.window.height = 720;

    ycode::Engine engine(config);
    auto& player = engine.scene().createEntity("Sandbox Player");
    player.transform.position = ycode::Vec2{0.0f, 0.0f};
    player.properties["kind"] = "prototype";
    ycode::EntityId playerId = player.id;

    engine.scene().setUpdateHandler([playerId](ycode::Scene& scene, float deltaSeconds) {
        auto* entity = scene.findEntity(playerId);
        if (!entity || !entity->active)
            return;

        entity->transform.position.x += 64.0f * deltaSeconds;
        if (entity->transform.position.x > 320.0f)
            entity->transform.position.x = 0.0f;
    });

    engine.events().subscribe("*", [](const ycode::Event& event) {
        if (event.type == "engine.tick")
            return;
        std::cout << "[event] " << event.type << std::endl;
    });

    std::string error;
    if (!engine.initialize(&error))
    {
        std::cerr << "Engine initialization failed: " << error << std::endl;
        return 1;
    }

    for (int frame = 0; engine.isRunning() && frame < 300; ++frame)
    {
        engine.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    if (auto* entity = engine.scene().findEntity(playerId))
        std::cout << "Sandbox Player x=" << entity->transform.position.x << std::endl;

    engine.shutdown();
    return 0;
}
