#ifndef YCODE_SCENE_H
#define YCODE_SCENE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ycode {

using EntityId = std::uint64_t;
constexpr EntityId kInvalidEntityId = 0;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Transform2D {
    Vec2 position;
    float rotationDegrees = 0.0f;
    Vec2 scale{1.0f, 1.0f};
};

struct Entity {
    EntityId id = kInvalidEntityId;
    std::string name;
    bool active = true;
    Transform2D transform;
    std::unordered_map<std::string, std::string> properties;
};

class Scene {
public:
    using UpdateHandler = std::function<void(Scene&, float)>;

    explicit Scene(std::string name = "Main Scene");

    Entity& createEntity(const std::string& name = {});
    bool destroyEntity(EntityId id);

    Entity* findEntity(EntityId id);
    const Entity* findEntity(EntityId id) const;
    Entity* findEntityByName(const std::string& name);
    const Entity* findEntityByName(const std::string& name) const;

    std::vector<Entity>& entities();
    const std::vector<Entity>& entities() const;
    std::size_t entityCount() const;
    bool empty() const;

    void clear();
    void update(float deltaSeconds);
    void setUpdateHandler(UpdateHandler handler);

    const std::string& name() const;
    void setName(std::string name);

private:
    std::string name_;
    EntityId nextEntityId_ = 1;
    std::vector<Entity> entities_;
    UpdateHandler updateHandler_;
};

} // namespace ycode

#endif // YCODE_SCENE_H
