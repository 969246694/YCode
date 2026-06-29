#include "ycode/core.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

void drawScene(ycode::Engine& engine, ycode::EntityId playerId, void* nativeDc, int width, int height)
{
    auto* entity = engine.scene().findEntity(playerId);
    if (!entity)
        return;

    ycode::Canvas2D canvas(nativeDc, width, height);
    float size = 48.0f * entity->transform.scale.x;
    if (size < 16)
        size = 16;

    float centerX = static_cast<float>(width) * 0.5f + entity->transform.position.x;
    float centerY = static_cast<float>(height) * 0.5f - entity->transform.position.y;
    float left = centerX - size * 0.5f;
    float top = centerY - size * 0.5f;

    canvas.fillRect(left, top, size, size, ycode::Color{54, 162, 235, 255});
    canvas.strokeRect(left, top, size, size, ycode::Color{235, 245, 255, 255}, 2);
}

} // namespace

int main()
{
    auto version = ycode::getVersion();
    std::cout << "YCode Engine v" << version.major << "." << version.minor << "." << version.patch << std::endl;
    std::cout << "================================" << std::endl;

    ycode::EngineConfig config;
    config.appName = "YCode Engine Sandbox";
    config.projectRoot = ".";
    config.startupScenePath = "scenes/sandbox.scene.json";
    config.loadStartupScene = true;
    config.window.title = "YCode Engine Sandbox";
    config.window.width = 1280;
    config.window.height = 720;

    ycode::Engine engine(config);
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

    auto* player = engine.scene().findEntityByName("Sandbox Player");
    if (!player)
    {
        std::cerr << "Sandbox scene does not contain 'Sandbox Player'" << std::endl;
        return 1;
    }

    ycode::EntityId playerId = player->id;
    engine.scene().setUpdateHandler([&engine, playerId](ycode::Scene& scene, float deltaSeconds) {
        auto* entity = scene.findEntity(playerId);
        if (!entity || !entity->active)
            return;

        float horizontal = 0.0f;
        float vertical = 0.0f;
        if (engine.window().isKeyDown(ycode::Key::Left) || engine.window().isKeyDown(ycode::Key::A))
            horizontal -= 1.0f;
        if (engine.window().isKeyDown(ycode::Key::Right) || engine.window().isKeyDown(ycode::Key::D))
            horizontal += 1.0f;
        if (engine.window().isKeyDown(ycode::Key::Down) || engine.window().isKeyDown(ycode::Key::S))
            vertical -= 1.0f;
        if (engine.window().isKeyDown(ycode::Key::Up) || engine.window().isKeyDown(ycode::Key::W))
            vertical += 1.0f;

        constexpr float speed = 220.0f;
        entity->transform.position.x += horizontal * speed * deltaSeconds;
        entity->transform.position.y += vertical * speed * deltaSeconds;
    });
    engine.window().setPaintHandler([&engine, playerId](void* nativeDc, int width, int height) {
        drawScene(engine, playerId, nativeDc, width, height);
    });
    engine.setRenderHandler([&engine]() {
        engine.window().invalidate();
    });

    for (int frame = 0; engine.isRunning() && frame < 300; ++frame)
    {
        engine.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    if (auto* entity = engine.scene().findEntity(playerId))
        std::cout << "Sandbox Player position=(" << entity->transform.position.x
                  << ", " << entity->transform.position.y << ")" << std::endl;

    engine.shutdown();
    return 0;
}
