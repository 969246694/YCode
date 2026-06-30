#include "ycode/core.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

void drawBox(ycode::Canvas2D& canvas,
             const ycode::Entity& entity,
             int width,
             int height,
             float boxWidth,
             float boxHeight,
             ycode::Color fill)
{
    float centerX = static_cast<float>(width) * 0.5f + entity.transform.position.x;
    float centerY = static_cast<float>(height) * 0.5f - entity.transform.position.y;
    float left = centerX - boxWidth * 0.5f;
    float top = centerY - boxHeight * 0.5f;

    canvas.fillRect(left, top, boxWidth, boxHeight, fill);
    canvas.strokeRect(left, top, boxWidth, boxHeight, ycode::Color{235, 245, 255, 255}, 2);
}

void drawScene(ycode::Engine& engine, ycode::EntityId playerId, ycode::EntityId groundId, void* nativeDc, int width, int height)
{
    ycode::Canvas2D canvas(nativeDc, width, height);

    if (auto* ground = engine.scene().findEntity(groundId))
        drawBox(canvas, *ground, width, height, 768.0f, 32.0f, ycode::Color{72, 92, 112, 255});

    if (auto* player = engine.scene().findEntity(playerId))
    {
        float size = 48.0f * player->transform.scale.x;
        if (size < 16.0f)
            size = 16.0f;
        drawBox(canvas, *player, width, height, size, size, ycode::Color{54, 162, 235, 255});
    }
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
    auto& ground = engine.scene().createEntity("Sandbox Ground");
    ground.transform.position = ycode::Vec2{0.0f, -160.0f};
    ycode::EntityId groundId = ground.id;

    ycode::BoxCollider2D playerCollider;
    playerCollider.halfSizeMeters = ycode::Vec2{0.375f, 0.375f};
    playerCollider.fixedRotation = true;
    if (!engine.physics().attachBox(engine.scene(), playerId, ycode::BodyType2D::Dynamic, playerCollider, &error))
    {
        std::cerr << "Failed to attach player physics body: " << error << std::endl;
        return 1;
    }

    ycode::BoxCollider2D groundCollider;
    groundCollider.halfSizeMeters = ycode::Vec2{6.0f, 0.25f};
    groundCollider.density = 0.0f;
    groundCollider.friction = 0.6f;
    if (!engine.physics().attachBox(engine.scene(), groundId, ycode::BodyType2D::Static, groundCollider, &error))
    {
        std::cerr << "Failed to attach ground physics body: " << error << std::endl;
        return 1;
    }

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

        ycode::Vec2 velocity = engine.physics().linearVelocity(playerId);
        velocity.x = horizontal * 4.0f;
        if (vertical != 0.0f)
            velocity.y = vertical * 4.0f;
        engine.physics().setLinearVelocity(playerId, velocity);
    });
    engine.window().setPaintHandler([&engine, playerId, groundId](void* nativeDc, int width, int height) {
        drawScene(engine, playerId, groundId, nativeDc, width, height);
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
