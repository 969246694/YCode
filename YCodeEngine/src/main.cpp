#include "ycode/core.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

constexpr int kKeyLeft = 0x25;
constexpr int kKeyUp = 0x26;
constexpr int kKeyRight = 0x27;
constexpr int kKeyDown = 0x28;
constexpr int kKeyA = 'A';
constexpr int kKeyD = 'D';
constexpr int kKeyS = 'S';
constexpr int kKeyW = 'W';

#ifdef _WIN32
void drawScene(ycode::Engine& engine, ycode::EntityId playerId, void* nativeDc, int width, int height)
{
    HDC dc = static_cast<HDC>(nativeDc);
    auto* entity = engine.scene().findEntity(playerId);
    if (!entity)
        return;

    int size = static_cast<int>(48.0f * entity->transform.scale.x);
    if (size < 16)
        size = 16;

    int centerX = width / 2 + static_cast<int>(entity->transform.position.x);
    int centerY = height / 2 - static_cast<int>(entity->transform.position.y);
    RECT playerRect = {
        centerX - size / 2,
        centerY - size / 2,
        centerX + size / 2,
        centerY + size / 2
    };

    HBRUSH playerBrush = CreateSolidBrush(RGB(54, 162, 235));
    FillRect(dc, &playerRect, playerBrush);
    DeleteObject(playerBrush);

    HPEN outlinePen = CreatePen(PS_SOLID, 2, RGB(235, 245, 255));
    HGDIOBJ oldPen = SelectObject(dc, outlinePen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, playerRect.left, playerRect.top, playerRect.right, playerRect.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(outlinePen);
}
#else
void drawScene(ycode::Engine&, ycode::EntityId, void*, int, int)
{
}
#endif

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
        if (engine.window().isKeyDown(kKeyLeft) || engine.window().isKeyDown(kKeyA))
            horizontal -= 1.0f;
        if (engine.window().isKeyDown(kKeyRight) || engine.window().isKeyDown(kKeyD))
            horizontal += 1.0f;
        if (engine.window().isKeyDown(kKeyDown) || engine.window().isKeyDown(kKeyS))
            vertical -= 1.0f;
        if (engine.window().isKeyDown(kKeyUp) || engine.window().isKeyDown(kKeyW))
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
