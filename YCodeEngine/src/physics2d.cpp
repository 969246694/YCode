#include "ycode/physics2d.h"

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace ycode {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMaxStepSeconds = 0.25f;

b2Vec2 toB2(Vec2 value)
{
    return b2Vec2{value.x, value.y};
}

Vec2 toYCode(b2Vec2 value)
{
    return Vec2{value.x, value.y};
}

float degreesToRadians(float degrees)
{
    return degrees * kPi / 180.0f;
}

float radiansToDegrees(float radians)
{
    return radians * 180.0f / kPi;
}

b2BodyType toB2(BodyType2D type)
{
    switch (type)
    {
    case BodyType2D::Static:
        return b2_staticBody;
    case BodyType2D::Kinematic:
        return b2_kinematicBody;
    case BodyType2D::Dynamic:
    default:
        return b2_dynamicBody;
    }
}

void setError(std::string* error, const std::string& message)
{
    if (error)
        *error = message;
}

} // namespace

struct PhysicsWorld2D::Impl {
    struct BodyBinding {
        EntityId entityId = kInvalidEntityId;
        BodyType2D type = BodyType2D::Dynamic;
        b2BodyId bodyId = b2_nullBodyId;
    };

    explicit Impl(PhysicsConfig2D initialConfig)
        : config(std::move(initialConfig))
    {
        createWorld();
    }

    ~Impl()
    {
        destroyWorld();
    }

    void createWorld()
    {
        if (B2_IS_NON_NULL(worldId))
            return;

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = toB2(config.gravity);
        worldId = b2CreateWorld(&worldDef);
    }

    void destroyWorld()
    {
        if (B2_IS_NON_NULL(worldId))
        {
            b2DestroyWorld(worldId);
            worldId = b2_nullWorldId;
        }
        bodies.clear();
    }

    float metersToPixels(float value) const
    {
        return value * config.pixelsPerMeter;
    }

    float pixelsToMeters(float value) const
    {
        return value / config.pixelsPerMeter;
    }

    b2Vec2 sceneToPhysics(Vec2 position) const
    {
        return b2Vec2{pixelsToMeters(position.x), pixelsToMeters(position.y)};
    }

    Vec2 physicsToScene(b2Vec2 position) const
    {
        return Vec2{metersToPixels(position.x), metersToPixels(position.y)};
    }

    std::vector<BodyBinding>::iterator find(EntityId entityId)
    {
        return std::find_if(bodies.begin(), bodies.end(),
                            [entityId](const BodyBinding& binding) {
                                return binding.entityId == entityId;
                            });
    }

    std::vector<BodyBinding>::const_iterator find(EntityId entityId) const
    {
        return std::find_if(bodies.begin(), bodies.end(),
                            [entityId](const BodyBinding& binding) {
                                return binding.entityId == entityId;
                            });
    }

    PhysicsConfig2D config;
    b2WorldId worldId = b2_nullWorldId;
    std::vector<BodyBinding> bodies;
};

PhysicsWorld2D::PhysicsWorld2D(PhysicsConfig2D config)
    : impl_(std::make_unique<Impl>(std::move(config)))
{
}

PhysicsWorld2D::~PhysicsWorld2D() = default;

PhysicsWorld2D::PhysicsWorld2D(PhysicsWorld2D&&) noexcept = default;

PhysicsWorld2D& PhysicsWorld2D::operator=(PhysicsWorld2D&&) noexcept = default;

void PhysicsWorld2D::configure(PhysicsConfig2D config)
{
    if (config.pixelsPerMeter <= 0.0f)
        config.pixelsPerMeter = 64.0f;
    if (config.subStepCount <= 0)
        config.subStepCount = 4;

    impl_->config = std::move(config);
    if (B2_IS_NON_NULL(impl_->worldId))
        b2World_SetGravity(impl_->worldId, toB2(impl_->config.gravity));
}

const PhysicsConfig2D& PhysicsWorld2D::config() const
{
    return impl_->config;
}

bool PhysicsWorld2D::isEnabled() const
{
    return impl_->config.enabled;
}

void PhysicsWorld2D::setEnabled(bool enabled)
{
    impl_->config.enabled = enabled;
}

bool PhysicsWorld2D::attachBox(Scene& scene,
                               EntityId entityId,
                               BodyType2D bodyType,
                               BoxCollider2D collider,
                               std::string* error)
{
    if (entityId == kInvalidEntityId)
    {
        setError(error, "entityId is invalid");
        return false;
    }

    Entity* entity = scene.findEntity(entityId);
    if (!entity)
    {
        setError(error, "entity was not found in the scene");
        return false;
    }

    if (collider.halfSizeMeters.x <= 0.0f || collider.halfSizeMeters.y <= 0.0f)
    {
        setError(error, "collider halfSizeMeters must be greater than zero");
        return false;
    }

    if (impl_->config.pixelsPerMeter <= 0.0f)
    {
        setError(error, "pixelsPerMeter must be greater than zero");
        return false;
    }

    impl_->createWorld();
    detach(entityId);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = toB2(bodyType);
    bodyDef.position = impl_->sceneToPhysics(entity->transform.position);
    bodyDef.rotation = b2MakeRot(degreesToRadians(entity->transform.rotationDegrees));
    bodyDef.fixedRotation = collider.fixedRotation;
    bodyDef.name = entity->name.c_str();

    b2BodyId bodyId = b2CreateBody(impl_->worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = collider.density;
    shapeDef.material.friction = collider.friction;
    shapeDef.material.restitution = collider.restitution;

    b2Polygon box = b2MakeBox(collider.halfSizeMeters.x, collider.halfSizeMeters.y);
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    impl_->bodies.push_back(Impl::BodyBinding{entityId, bodyType, bodyId});
    return true;
}

bool PhysicsWorld2D::detach(EntityId entityId)
{
    auto it = impl_->find(entityId);
    if (it == impl_->bodies.end())
        return false;

    if (B2_IS_NON_NULL(it->bodyId) && b2Body_IsValid(it->bodyId))
        b2DestroyBody(it->bodyId);

    impl_->bodies.erase(it);
    return true;
}

bool PhysicsWorld2D::hasBody(EntityId entityId) const
{
    return impl_->find(entityId) != impl_->bodies.end();
}

void PhysicsWorld2D::clear()
{
    impl_->destroyWorld();
    impl_->createWorld();
}

void PhysicsWorld2D::step(Scene& scene, float deltaSeconds)
{
    if (!impl_->config.enabled || deltaSeconds <= 0.0f || impl_->bodies.empty())
        return;

    if (deltaSeconds > kMaxStepSeconds)
        deltaSeconds = kMaxStepSeconds;

    for (auto it = impl_->bodies.begin(); it != impl_->bodies.end();)
    {
        Entity* entity = scene.findEntity(it->entityId);
        if (!entity || !b2Body_IsValid(it->bodyId))
        {
            if (B2_IS_NON_NULL(it->bodyId) && b2Body_IsValid(it->bodyId))
                b2DestroyBody(it->bodyId);
            it = impl_->bodies.erase(it);
            continue;
        }

        if (it->type != BodyType2D::Dynamic)
        {
            b2Body_SetTransform(it->bodyId,
                                impl_->sceneToPhysics(entity->transform.position),
                                b2MakeRot(degreesToRadians(entity->transform.rotationDegrees)));
        }

        ++it;
    }

    if (impl_->bodies.empty())
        return;

    b2World_Step(impl_->worldId, deltaSeconds, impl_->config.subStepCount);

    for (const auto& binding : impl_->bodies)
    {
        Entity* entity = scene.findEntity(binding.entityId);
        if (!entity || !b2Body_IsValid(binding.bodyId))
            continue;

        b2Vec2 position = b2Body_GetPosition(binding.bodyId);
        b2Rot rotation = b2Body_GetRotation(binding.bodyId);
        entity->transform.position = impl_->physicsToScene(position);
        entity->transform.rotationDegrees = radiansToDegrees(b2Rot_GetAngle(rotation));
    }
}

bool PhysicsWorld2D::setLinearVelocity(EntityId entityId, Vec2 metersPerSecond)
{
    auto it = impl_->find(entityId);
    if (it == impl_->bodies.end() || !b2Body_IsValid(it->bodyId))
        return false;

    b2Body_SetLinearVelocity(it->bodyId, toB2(metersPerSecond));
    return true;
}

Vec2 PhysicsWorld2D::linearVelocity(EntityId entityId) const
{
    auto it = impl_->find(entityId);
    if (it == impl_->bodies.end() || !b2Body_IsValid(it->bodyId))
        return {};

    return toYCode(b2Body_GetLinearVelocity(it->bodyId));
}

} // namespace ycode
