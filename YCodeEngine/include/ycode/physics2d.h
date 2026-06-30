#ifndef YCODE_PHYSICS2D_H
#define YCODE_PHYSICS2D_H

#include "ycode/scene.h"

#include <cstddef>
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
    bool attachSceneBodies(Scene& scene, std::string* error = nullptr);
    bool detach(EntityId entityId);
    bool hasBody(EntityId entityId) const;
    std::size_t bodyCount() const;
    void clear();

    void step(Scene& scene, float deltaSeconds);

    bool setLinearVelocity(EntityId entityId, Vec2 metersPerSecond);
    Vec2 linearVelocity(EntityId entityId) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ycode

#endif // YCODE_PHYSICS2D_H
