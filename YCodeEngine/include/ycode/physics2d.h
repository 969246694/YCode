#ifndef YCODE_PHYSICS2D_H
#define YCODE_PHYSICS2D_H

#include "ycode/scene.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace ycode {

struct PhysicsConfig2D {
    Vec2 gravity{0.0f, -9.8f};
    float pixelsPerMeter = 64.0f;
    int subStepCount = 4;
    bool enabled = true;
};

class PhysicsWorld2D {
public:
    explicit PhysicsWorld2D(PhysicsConfig2D config = {});
    ~PhysicsWorld2D();

    PhysicsWorld2D(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D(PhysicsWorld2D&&) noexcept;
    PhysicsWorld2D& operator=(PhysicsWorld2D&&) noexcept;

    void configure(PhysicsConfig2D config);
    const PhysicsConfig2D& config() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool attachBox(Scene& scene,
                   EntityId entityId,
                   BodyType2D bodyType,
                   BoxCollider2D collider = {},
                   std::string* error = nullptr);
    bool attachCircle(Scene& scene,
                      EntityId entityId,
                      BodyType2D bodyType,
                      CircleCollider2D collider = {},
                      std::string* error = nullptr);
    bool attachCapsule(Scene& scene,
                       EntityId entityId,
                       BodyType2D bodyType,
                       CapsuleCollider2D collider = {},
                       std::string* error = nullptr);
    bool attachSceneBodies(Scene& scene, std::string* error = nullptr);
    bool detach(EntityId entityId);
    bool hasBody(EntityId entityId) const;
    std::size_t bodyCount() const;
    void clear();

    void step(Scene& scene, float deltaSeconds);

    // 碰撞接触事件：每次 step 后触发，begin=true 表示接触开始、false 表示接触结束。
    // 参数为发生接触的两个实体 id。
    using ContactHandler = std::function<void(EntityId a, EntityId b, bool begin)>;
    void setContactHandler(ContactHandler handler);

    // 命中事件：两个形状以超过阈值速度碰撞时触发，给出碰撞点（像素）、法线与接近速度（米/秒）。
    using HitHandler = std::function<void(EntityId a, EntityId b, Vec2 point, Vec2 normal, float approachSpeed)>;
    void setHitHandler(HitHandler handler);

    // 射线检测（场景像素坐标）：从 from 射向 to，返回命中的第一个实体 id；
    // 未命中返回 kInvalidEntityId；可选输出命中点（像素坐标）。
    EntityId castRay(const Vec2& from, const Vec2& to, Vec2* hitPoint = nullptr) const;

    bool setLinearVelocity(EntityId entityId, Vec2 metersPerSecond);
    Vec2 linearVelocity(EntityId entityId) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    ContactHandler contactHandler_;
    HitHandler hitHandler_;
};

} // namespace ycode

#endif // YCODE_PHYSICS2D_H
