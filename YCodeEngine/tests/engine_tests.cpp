#include "ycode/engine.h"
#include "ycode/event_bus.h"
#include "ycode/physics2d.h"
#include "ycode/resource_manager.h"
#include "ycode/scene.h"
#include "ycode/scene_loader.h"
#include "ycode/scene_saver.h"
#include "ycode/texture.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

} // namespace

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond)) {                                                     \
            ++g_failures;                                                  \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                  \
    } while (0)

#define CHECK_EQ(a, b)                                                     \
    do {                                                                   \
        ++g_checks;                                                        \
        auto _a = (a);                                                     \
        auto _b = (b);                                                     \
        if (!(_a == _b)) {                                                 \
            ++g_failures;                                                  \
            std::printf("FAIL %s:%d  %s == %s\n", __FILE__, __LINE__, #a, #b); \
        }                                                                  \
    } while (0)

#define CHECK_NEAR(a, b, eps)                                              \
    do {                                                                   \
        ++g_checks;                                                        \
        float _a = static_cast<float>(a);                                  \
        float _b = static_cast<float>(b);                                  \
        float _e = static_cast<float>(eps);                                \
        if (std::fabs(_a - _b) > _e) {                                     \
            ++g_failures;                                                  \
            std::printf("FAIL %s:%d  %s ~= %s\n", __FILE__, __LINE__, #a, #b); \
        }                                                                  \
    } while (0)

// ------------------------------------------------------------
// EventBus
// ------------------------------------------------------------
static void testEventBus()
{
    ycode::EventBus bus;
    int specific = 0;
    int wildcard = 0;

    bus.subscribe("ping", [&](const ycode::Event&) { ++specific; });
    bus.subscribe("*", [&](const ycode::Event&) { ++wildcard; });

    bus.publish({"ping", {}});
    CHECK_EQ(specific, 1);
    CHECK_EQ(wildcard, 1);

    bus.publish({"other", {}});
    CHECK_EQ(specific, 1);   // 未匹配具体类型
    CHECK_EQ(wildcard, 2);   // 通配符仍收到

    ycode::EventBus::SubscriptionId id =
        bus.subscribe("ping", [&](const ycode::Event&) { specific += 100; });
    bus.publish({"ping", {}});
    CHECK_EQ(specific, 102); // 1 + 1 + 100

    CHECK(bus.unsubscribe(id));
    CHECK(!bus.unsubscribe(id)); // 二次退订返回 false
    bus.publish({"ping", {}});
    CHECK_EQ(specific, 103); // 退订后不再累加 100

    bus.clear();
    bus.publish({"ping", {}});
    CHECK_EQ(specific, 103);
    CHECK_EQ(wildcard, 4);
}

static void testEventBusReentrancy()
{
    ycode::EventBus bus;
    int a = 0;
    int b = 0;

    // 处理函数在派发过程中退订自身：快照机制下不应导致迭代器失效。
    ycode::EventBus::SubscriptionId idA =
        bus.subscribe("t", [&](const ycode::Event&) {
            ++a;
            bus.unsubscribe(idA);
        });
    bus.subscribe("t", [&](const ycode::Event&) { ++b; });

    bus.publish({"t", {}});
    CHECK_EQ(a, 1);
    CHECK_EQ(b, 1);

    bus.publish({"t", {}});
    CHECK_EQ(a, 1); // 已在上一轮退订
    CHECK_EQ(b, 2);
}

// ------------------------------------------------------------
// Scene
// ------------------------------------------------------------
static void testScene()
{
    ycode::Scene scene("Test");
    CHECK_EQ(scene.name(), std::string("Test"));
    CHECK(scene.empty());

    // 注意：createEntity 返回 vector 元素引用，后续 push_back 可能使其失效，
    // 所以这里只持有 id 值，再用 findEntity 重新定位。
    ycode::EntityId aId = scene.createEntity("A").id;
    ycode::EntityId bId = scene.createEntity().id;
    CHECK_EQ(scene.entityCount(), std::size_t(2));
    CHECK(aId != bId);

    ycode::Entity* b = scene.findEntity(bId);
    CHECK(b != nullptr);
    if (b)
        CHECK_EQ(b->name, std::string("Entity 2")); // 默认命名

    ycode::Entity* a = scene.findEntityByName("A");
    CHECK(a != nullptr);
    if (a)
        CHECK_EQ(a->id, aId);

    CHECK(scene.findEntity(aId) != nullptr);
    CHECK_EQ(scene.findEntity(999), nullptr);
    CHECK_EQ(scene.findEntityByName("nope"), nullptr);

    CHECK(scene.destroyEntity(aId));
    CHECK_EQ(scene.entityCount(), std::size_t(1));
    CHECK_EQ(scene.findEntity(aId), nullptr);
    CHECK(!scene.destroyEntity(aId)); // 已删除

    int calls = 0;
    scene.setUpdateHandler([&](ycode::Scene&, float) { ++calls; });
    scene.update(0.016f);
    CHECK_EQ(calls, 1);

    scene.clear();
    CHECK(scene.empty());
    CHECK_EQ(scene.entityCount(), std::size_t(0));

    // clear 后 id 从 1 重新分配
    ycode::EntityId cId = scene.createEntity().id;
    CHECK_EQ(cId, ycode::EntityId(1));
}

// ------------------------------------------------------------
// SceneLoader
// ------------------------------------------------------------
static void testSceneLoader()
{
    ycode::Scene scene;
    std::string err;
    const char* json = R"({
      "name": "Loaded",
      "entities": [
        {
          "name": "Player",
          "transform": { "position": [10, 20], "rotationDegrees": 45, "scale": [2, 3] },
          "physics2D": {
            "bodyType": "dynamic",
            "box": { "halfSizeMeters": [0.4, 0.6], "fixedRotation": true }
          },
          "properties": { "kind": "hero", "hp": 100, "alive": true, "nick": null, "speed": 1.5 }
        },
        {
          "name": "Ground",
          "physics2D": { "bodyType": "static", "box": { "density": 0.0, "friction": 0.6 } }
        },
        {
          "name": "Coin",
          "transform": { "position": [5, 10] },
          "physics2D": {
            "bodyType": "kinematic",
            "circle": { "radiusMeters": 0.3, "friction": 0.1 }
          }
        },
        {
          "name": "Hero",
          "transform": { "position": [8, 15] },
          "physics2D": {
            "bodyType": "dynamic",
            "capsule": { "center1": [0, -0.5], "center2": [0, 0.5], "radiusMeters": 0.2 }
          }
        }
      ]
    })";

    CHECK(ycode::SceneLoader::loadFromText(json, scene, &err));
    CHECK_EQ(scene.name(), std::string("Loaded"));
    CHECK_EQ(scene.entityCount(), std::size_t(4));

    ycode::Entity* player = scene.findEntityByName("Player");
    if (player)
    {
        CHECK_NEAR(player->transform.position.x, 10.0f, 1e-5f);
        CHECK_NEAR(player->transform.position.y, 20.0f, 1e-5f);
        CHECK_NEAR(player->transform.rotationDegrees, 45.0f, 1e-5f);
        CHECK_NEAR(player->transform.scale.x, 2.0f, 1e-5f);
        CHECK_NEAR(player->transform.scale.y, 3.0f, 1e-5f);

        CHECK(player->physics2D.enabled);
        CHECK(player->physics2D.bodyType == ycode::BodyType2D::Dynamic);
        CHECK_NEAR(player->physics2D.box.halfSizeMeters.x, 0.4f, 1e-5f);
        CHECK_NEAR(player->physics2D.box.halfSizeMeters.y, 0.6f, 1e-5f);
        CHECK(player->physics2D.box.fixedRotation);

        CHECK_EQ(player->properties.at("kind"), std::string("hero"));
        CHECK_EQ(player->properties.at("hp"), std::string("100"));
        CHECK_EQ(player->properties.at("alive"), std::string("true"));
        CHECK_EQ(player->properties.at("nick"), std::string(""));
        CHECK_EQ(player->properties.at("speed"), std::string("1.5"));
    }

    ycode::Entity* ground = scene.findEntityByName("Ground");
    if (ground)
    {
        CHECK(ground->physics2D.bodyType == ycode::BodyType2D::Static);
        CHECK_NEAR(ground->physics2D.box.density, 0.0f, 1e-5f);
        CHECK_NEAR(ground->physics2D.box.friction, 0.6f, 1e-5f);
    }

    ycode::Entity* coin = scene.findEntityByName("Coin");
    if (coin)
    {
        CHECK(coin->physics2D.useCircle);
        CHECK(coin->physics2D.bodyType == ycode::BodyType2D::Kinematic);
        CHECK_NEAR(coin->physics2D.circle.radiusMeters, 0.3f, 1e-5f);
        CHECK_NEAR(coin->physics2D.circle.friction, 0.1f, 1e-5f);
    }

    ycode::Entity* hero = scene.findEntityByName("Hero");
    if (hero)
    {
        CHECK(hero->physics2D.useCapsule);
        CHECK(hero->physics2D.bodyType == ycode::BodyType2D::Dynamic);
        CHECK_NEAR(hero->physics2D.capsule.radiusMeters, 0.2f, 1e-5f);
        CHECK_NEAR(hero->physics2D.capsule.center1.y, -0.5f, 1e-5f);
        CHECK_NEAR(hero->physics2D.capsule.center2.y, 0.5f, 1e-5f);
    }

    // 错误路径
    ycode::Scene s1;
    CHECK(!ycode::SceneLoader::loadFromText("not json", s1, &err));

    ycode::Scene s2;
    CHECK(!ycode::SceneLoader::loadFromText("{}", s2, &err)); // 缺少 entities 数组

    ycode::Scene s3;
    CHECK(!ycode::SceneLoader::loadFromText(
        "{\"entities\":[{\"physics2D\":{\"bodyType\":\"nope\"}}]}", s3, &err));

    ycode::Scene s4;
    CHECK(!ycode::SceneLoader::loadFromText(
        "{\"entities\":[{\"transform\":{\"position\":[1]}}]}", s4, &err)); // vec2 长度错误
}

// ------------------------------------------------------------
// ResourceManager
// ------------------------------------------------------------
static void testResourceManager()
{
    ycode::ResourceManager rm(".");

    std::string rel = rm.resolvePath("scenes/main.scene.json");
    CHECK(!rel.empty());
    CHECK(rel.find("scenes") != std::string::npos);

    // 传入已解析的相对路径应稳定（幂等）
    CHECK_EQ(rm.resolvePath(rel), rel);

    CHECK(!rm.exists("__definitely_missing__.json"));

    std::string out;
    std::string err;
    CHECK(!rm.readText("__definitely_missing__.json", out, &err));
    CHECK(!err.empty());
}

// ------------------------------------------------------------
// PhysicsWorld2D
// ------------------------------------------------------------
static void testPhysics()
{
    ycode::Scene scene;
    ycode::Entity& e = scene.createEntity("Ball");
    e.transform.position = ycode::Vec2{0.0f, 0.0f};

    ycode::PhysicsWorld2D physics;
    CHECK(physics.isEnabled());
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    ycode::BoxCollider2D box;
    box.halfSizeMeters = ycode::Vec2{0.5f, 0.5f};
    std::string err;
    CHECK(physics.attachBox(scene, e.id, ycode::BodyType2D::Dynamic, box, &err));
    CHECK(physics.hasBody(e.id));
    CHECK_EQ(physics.bodyCount(), std::size_t(1));

    // 速度读写往返
    physics.setLinearVelocity(e.id, ycode::Vec2{1.0f, 2.0f});
    ycode::Vec2 v = physics.linearVelocity(e.id);
    CHECK_NEAR(v.x, 1.0f, 1e-4f);
    CHECK_NEAR(v.y, 2.0f, 1e-4f);

    // 受重力下落：动态刚体的 y 位置应随时间减小
    float y0 = e.transform.position.y;
    for (int i = 0; i < 60; ++i)
        physics.step(scene, 1.0f / 60.0f);
    CHECK(e.transform.position.y < y0);

    // 同一实体重新挂接会替换旧刚体
    CHECK(physics.attachBox(scene, e.id, ycode::BodyType2D::Static, box, &err));
    CHECK_EQ(physics.bodyCount(), std::size_t(1));

    CHECK(physics.detach(e.id));
    CHECK(!physics.hasBody(e.id));
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    // 非法参数
    ycode::BoxCollider2D bad;
    bad.halfSizeMeters = ycode::Vec2{0.0f, 0.5f};
    CHECK(!physics.attachBox(scene, e.id, ycode::BodyType2D::Dynamic, bad, &err));
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    physics.clear();
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    // 圆形碰撞体
    ycode::Entity& ball = scene.createEntity("CircleBall");
    ball.transform.position = ycode::Vec2{100.0f, 0.0f};
    ycode::CircleCollider2D circle;
    circle.radiusMeters = 0.25f;
    CHECK(physics.attachCircle(scene, ball.id, ycode::BodyType2D::Dynamic, circle, &err));
    CHECK(physics.hasBody(ball.id));
    CHECK_EQ(physics.bodyCount(), std::size_t(1));

    CHECK(physics.detach(ball.id));
    CHECK(!physics.hasBody(ball.id));
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    // 无效半径
    ycode::CircleCollider2D badCircle;
    badCircle.radiusMeters = 0.0f;
    CHECK(!physics.attachCircle(scene, ball.id, ycode::BodyType2D::Dynamic, badCircle, &err));

    physics.clear();
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    // 胶囊碰撞体
    ycode::Entity& cap = scene.createEntity("CapsuleBody");
    cap.transform.position = ycode::Vec2{200.0f, 0.0f};
    ycode::CapsuleCollider2D capsule;
    capsule.radiusMeters = 0.2f;
    capsule.center1 = ycode::Vec2{0.0f, -0.4f};
    capsule.center2 = ycode::Vec2{0.0f, 0.4f};
    CHECK(physics.attachCapsule(scene, cap.id, ycode::BodyType2D::Dynamic, capsule, &err));
    CHECK(physics.hasBody(cap.id));
    CHECK_EQ(physics.bodyCount(), std::size_t(1));

    CHECK(physics.detach(cap.id));
    CHECK(!physics.hasBody(cap.id));
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    // 无效半径
    ycode::CapsuleCollider2D badCapsule;
    badCapsule.radiusMeters = 0.0f;
    CHECK(!physics.attachCapsule(scene, cap.id, ycode::BodyType2D::Dynamic, badCapsule, &err));

    physics.clear();
    CHECK_EQ(physics.bodyCount(), std::size_t(0));

    // 碰撞接触事件：动态球落到静态地面上应触发 begin 接触
    {
        ycode::Scene contactScene;
        ycode::EntityId groundId = contactScene.createEntity("Ground").id;
        contactScene.findEntity(groundId)->transform.position = ycode::Vec2{0.0f, 0.0f};
        ycode::EntityId ballId = contactScene.createEntity("Ball").id;
        contactScene.findEntity(ballId)->transform.position = ycode::Vec2{0.0f, 300.0f};

        ycode::PhysicsWorld2D cPhysics;
        CHECK(cPhysics.attachBox(contactScene, groundId, ycode::BodyType2D::Static, {}, &err));
        CHECK(cPhysics.attachBox(contactScene, ballId, ycode::BodyType2D::Dynamic, {}, &err));

        int beginCount = 0;
        int hitCount = 0;
        bool sawBall = false;
        bool sawGround = false;
        cPhysics.setContactHandler([&](ycode::EntityId a, ycode::EntityId b, bool begin) {
            if (begin)
            {
                ++beginCount;
                if (a == ballId || b == ballId) sawBall = true;
                if (a == groundId || b == groundId) sawGround = true;
            }
        });
        cPhysics.setHitHandler([&](ycode::EntityId, ycode::EntityId, ycode::Vec2, ycode::Vec2, float) {
            ++hitCount;
        });

        // 让球下落约 1 秒
        for (int i = 0; i < 120; ++i)
            cPhysics.step(contactScene, 1.0f / 60.0f);

        CHECK(beginCount > 0);
        CHECK(sawBall && sawGround);
        CHECK(hitCount > 0); // 球以 >1 m/s 落地，应触发命中事件
    }

    // 射线检测：穿过静态盒子应命中，偏离应未命中
    {
        ycode::Scene rayScene;
        ycode::EntityId target = rayScene.createEntity("Target").id;
        rayScene.findEntity(target)->transform.position = ycode::Vec2{50.0f, 0.0f};

        ycode::PhysicsWorld2D rPhysics;
        CHECK(rPhysics.attachBox(rayScene, target, ycode::BodyType2D::Static, {}, &err));

        ycode::Vec2 hit;
        ycode::EntityId hitId = rPhysics.castRay(ycode::Vec2{0.0f, 0.0f}, ycode::Vec2{100.0f, 0.0f}, &hit);
        CHECK(hitId == target);
        CHECK(hit.x > 0.0f && hit.x < 100.0f);

        ycode::EntityId hitId2 = rPhysics.castRay(ycode::Vec2{200.0f, 0.0f}, ycode::Vec2{0.0f, 0.0f}, nullptr);
        CHECK(hitId2 == target);

        ycode::EntityId miss = rPhysics.castRay(ycode::Vec2{0.0f, 500.0f}, ycode::Vec2{100.0f, 500.0f}, nullptr);
        CHECK(miss == ycode::kInvalidEntityId);
    }
}

// ------------------------------------------------------------
// 文件读写路径（ResourceManager::readText / SceneLoader::loadFromFile）
// ------------------------------------------------------------
static void testFileIo()
{
    const char* textPath = "ycode_engine_test_tmp.txt";
    {
        std::ofstream f(textPath, std::ios::binary);
        f << "hello ycode";
    }

    ycode::ResourceManager rm(".");
    CHECK(rm.exists(textPath));
    std::string out;
    std::string err;
    CHECK(rm.readText(textPath, out, &err));
    CHECK_EQ(out, std::string("hello ycode"));
    std::remove(textPath);

    const char* scenePath = "ycode_engine_test_scene.json";
    {
        std::ofstream f(scenePath, std::ios::binary);
        f << R"({"name":"FileScene","entities":[{"name":"E"}]})";
    }
    ycode::Scene scene;
    CHECK(ycode::SceneLoader::loadFromFile(scenePath, scene, &err));
    CHECK_EQ(scene.name(), std::string("FileScene"));
    CHECK_EQ(scene.entityCount(), std::size_t(1));
    std::remove(scenePath);

    // 文件不存在
    ycode::Scene missing;
    CHECK(!ycode::SceneLoader::loadFromFile("__no_such_scene__.json", missing, &err));
}

// ------------------------------------------------------------
// Engine 生命周期（无窗口，仅验证 init/tick/shutdown）
// ------------------------------------------------------------
static void testEngineLifecycle()
{
    ycode::EngineConfig config;
    config.createWindow = false;
    config.loadStartupScene = false;

    ycode::Engine engine(config);
    std::string err;
    CHECK(engine.initialize(&err));
    CHECK(engine.isRunning());

    for (int i = 0; i < 5; ++i)
        engine.tick();

    engine.shutdown();
    CHECK(!engine.isRunning());
}

// ------------------------------------------------------------
// SceneSaver 往返：加载 -> 保存 -> 再加载，比对数据
// ------------------------------------------------------------
static void testSceneSaverRoundTrip()
{
    const char* srcJson = R"({
      "name": "RT",
      "entities": [
        {"name":"A","transform":{"position":[10,20]},"physics2D":{"bodyType":"dynamic","circle":{"radiusMeters":0.4}},"properties":{"kind":"a"}},
        {"name":"B","transform":{"position":[-5,3]},"physics2D":{"bodyType":"static","box":{"halfSizeMeters":[1,2]}}}
      ]
    })";

    ycode::Scene loaded;
    std::string err;
    CHECK(ycode::SceneLoader::loadFromText(srcJson, loaded, &err));

    std::string saved = ycode::SceneSaver::saveToText(loaded);
    CHECK(saved.find("RT") != std::string::npos);

    ycode::Scene loaded2;
    CHECK(ycode::SceneLoader::loadFromText(saved, loaded2, &err));
    CHECK_EQ(loaded2.name(), std::string("RT"));
    CHECK_EQ(loaded2.entityCount(), std::size_t(2));

    ycode::Entity* a = loaded2.findEntityByName("A");
    if (a)
    {
        CHECK_NEAR(a->transform.position.x, 10.0f, 1e-4f);
        CHECK_NEAR(a->transform.position.y, 20.0f, 1e-4f);
        CHECK(a->physics2D.useCircle);
        CHECK_NEAR(a->physics2D.circle.radiusMeters, 0.4f, 1e-5f);
        CHECK_EQ(a->properties.at("kind"), std::string("a"));
    }

    ycode::Entity* b = loaded2.findEntityByName("B");
    if (b)
    {
        CHECK(!b->physics2D.useCircle);
        CHECK(!b->physics2D.useCapsule);
        CHECK_NEAR(b->physics2D.box.halfSizeMeters.x, 1.0f, 1e-5f);
        CHECK_NEAR(b->physics2D.box.halfSizeMeters.y, 2.0f, 1e-5f);
    }

    // 写文件往返
    const char* path = "ycode_engine_test_scene_save.json";
    CHECK(ycode::SceneSaver::saveToFile(path, loaded, &err));
    ycode::Scene loaded3;
    CHECK(ycode::SceneLoader::loadFromFile(path, loaded3, &err));
    CHECK_EQ(loaded3.entityCount(), std::size_t(2));
    std::remove(path);
}

// ------------------------------------------------------------
// 贴图加载（GDI+ 解码 BMP）与按属性检索实体
// ------------------------------------------------------------
static bool writeTestBmp(const char* path, int w, int h)
{
    const int rowSize = ((w * 3 + 3) / 4) * 4;
    const int dataSize = rowSize * h;
    const int fileSize = 14 + 40 + dataSize;
    std::ofstream f(path, std::ios::binary);
    if (!f)
        return false;

    unsigned char header[14] = {'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0};
    header[2] = static_cast<unsigned char>(fileSize & 0xFF);
    header[3] = static_cast<unsigned char>((fileSize >> 8) & 0xFF);
    header[4] = static_cast<unsigned char>((fileSize >> 16) & 0xFF);
    header[5] = static_cast<unsigned char>((fileSize >> 24) & 0xFF);
    f.write(reinterpret_cast<const char*>(header), 14);

    unsigned char info[40] = {0};
    info[0] = 40;
    info[4] = static_cast<unsigned char>(w & 0xFF);
    info[5] = static_cast<unsigned char>((w >> 8) & 0xFF);
    info[8] = static_cast<unsigned char>(h & 0xFF);
    info[9] = static_cast<unsigned char>((h >> 8) & 0xFF);
    info[12] = 1;  // planes
    info[14] = 24; // bpp
    f.write(reinterpret_cast<const char*>(info), 40);

    std::vector<unsigned char> row(static_cast<size_t>(rowSize), 0);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            row[static_cast<size_t>(x) * 3 + 0] = 0;   // B
            row[static_cast<size_t>(x) * 3 + 1] = 0;   // G
            row[static_cast<size_t>(x) * 3 + 2] = 255; // R
        }
        f.write(reinterpret_cast<const char*>(row.data()), rowSize);
    }
    return true;
}

static void testTextureAndProperties()
{
    const char* bmpPath = "ycode_engine_test_tmp.bmp";
    if (writeTestBmp(bmpPath, 8, 6))
    {
        ycode::Texture2D tex;
#ifdef _WIN32
        CHECK(tex.loadFromFile(bmpPath));
        CHECK(tex.valid());
        CHECK_EQ(tex.width(), 8);
        CHECK_EQ(tex.height(), 6);
#else
        CHECK(!tex.loadFromFile(bmpPath)); // 非 Windows 平台无 GDI+，不支持加载
        CHECK(!tex.valid());
#endif
    }
    std::remove(bmpPath);

    ycode::Texture2D bad;
    CHECK(!bad.loadFromFile("__no_such_image__.png"));
    CHECK(!bad.valid());

    ycode::Scene scene;
    ycode::Entity& a = scene.createEntity("Hero");
    a.properties["kind"] = "hero";
    a.properties["hp"] = "100";
    ycode::Entity& b = scene.createEntity("Monster");
    b.properties["kind"] = "monster";
    ycode::Entity& c = scene.createEntity("Hero2");
    c.properties["kind"] = "hero";

    auto heroes = scene.findEntitiesByProperty("kind", "hero");
    CHECK_EQ(heroes.size(), std::size_t(2));
    CHECK_EQ(scene.findEntitiesByProperty("kind", "monster").size(), std::size_t(1));
    CHECK_EQ(scene.findEntitiesByProperty("kind", "nope").size(), std::size_t(0));
    CHECK_EQ(scene.findEntitiesByProperty("hp", "100").size(), std::size_t(1));
}

int main()
{
    testEventBus();
    testEventBusReentrancy();
    testScene();
    testSceneLoader();
    testResourceManager();
    testPhysics();
    testEngineLifecycle();
    testFileIo();
    testTextureAndProperties();
    testSceneSaverRoundTrip();

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    if (g_failures > 0)
        std::printf("SOME TESTS FAILED\n");
    else
        std::printf("ALL TESTS PASSED\n");
    return g_failures == 0 ? 0 : 1;
}
